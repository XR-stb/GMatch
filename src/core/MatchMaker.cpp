#include "MatchMaker.h"
#include <algorithm>
#include <cmath>
#include <thread>
#include <chrono>
#include <iostream>

namespace gmatch {

// RatingBasedStrategy 实现
RatingBasedStrategy::RatingBasedStrategy(int maxRatingDiff)
    : maxRatingDiff_(maxRatingDiff) {
}

bool RatingBasedStrategy::isMatch(const PlayerPtr& player1, const PlayerPtr& player2) const {
    int ratingDiff = std::abs(player1->getRating() - player2->getRating());
    return ratingDiff <= maxRatingDiff_;
}

// MatchQueue 实现
MatchQueue::MatchQueue()
    : matchStrategy_(std::make_shared<RatingBasedStrategy>()) {
}

void MatchQueue::addPlayer(const PlayerPtr& player) {
    std::lock_guard<std::mutex> lock(mutex_);
    // 不修改玩家状态，只添加到队列
    queue_.push_back(player);
}

void MatchQueue::removePlayer(Player::PlayerId playerId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find_if(queue_.begin(), queue_.end(),
        [playerId](const PlayerPtr& player) {
            return player->getId() == playerId;
        });
    
    if (it != queue_.end()) {
        // 不修改玩家状态，只从队列中移除
        queue_.erase(it);
    }
}

bool MatchQueue::tryMatchPlayers(std::vector<PlayerPtr>& matchedPlayers, int requiredPlayers, bool forceMatchOnTimeout, uint64_t timeoutThreshold) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (queue_.size() < requiredPlayers) {
        return false;
    }
    
    // 找到第一个玩家
    matchedPlayers.clear();
    matchedPlayers.push_back(queue_[0]);
    
    // 尝试匹配剩下的玩家
    for (size_t i = 1; i < queue_.size() && matchedPlayers.size() < requiredPlayers; ++i) {
        bool canMatch = true;
        
        // 检查与所有已匹配玩家的匹配度
        for (const auto& matchedPlayer : matchedPlayers) {
            if (!matchStrategy_->isMatch(matchedPlayer, queue_[i])) {
                canMatch = false;
                break;
            }
        }
        
        if (canMatch) {
            matchedPlayers.push_back(queue_[i]);
        }
    }
    
    // 如果找不到足够匹配的玩家，但队列中有足够多的玩家，且启用了超时匹配
    if (matchedPlayers.size() < requiredPlayers && queue_.size() >= requiredPlayers && forceMatchOnTimeout) {
        // 获取当前时间
        auto now = std::chrono::system_clock::now();
        auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
            
        // 检查第一个玩家的等待时间是否超过阈值
        if (nowMs - queue_[0]->getLastActivityTime() > timeoutThreshold) {
            std::cout << "Force matching due to timeout: " << 
                         (nowMs - queue_[0]->getLastActivityTime()) << "ms > " << 
                         timeoutThreshold << "ms" << std::endl;
            
            // 重置匹配列表，使用贪婪算法
            matchedPlayers.clear();
            for (size_t i = 0; i < requiredPlayers && i < queue_.size(); ++i) {
                matchedPlayers.push_back(queue_[i]);
            }
        }
    }
    
    // 如果匹配成功，从队列中移除这些玩家
    if (matchedPlayers.size() == requiredPlayers) {
        for (const auto& player : matchedPlayers) {
            player->setStatus(false);
            auto it = std::find_if(queue_.begin(), queue_.end(),
                [player](const PlayerPtr& p) {
                    return p->getId() == player->getId();
                });
            if (it != queue_.end()) {
                queue_.erase(it);
            }
        }
        return true;
    }
    
    matchedPlayers.clear();
    return false;
}

size_t MatchQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

void MatchQueue::setMatchStrategy(std::shared_ptr<MatchStrategy> strategy) {
    std::lock_guard<std::mutex> lock(mutex_);
    matchStrategy_ = strategy;
}

void MatchQueue::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& player : queue_) {
        player->setStatus(false);
    }
    queue_.clear();
}

// MatchMaker 实现
MatchMaker::MatchMaker(int playersPerRoom)
    : playersPerRoom_(playersPerRoom) {
}

MatchMaker::~MatchMaker() {
    stop();
}

void MatchMaker::start() {
    if (!running_) {
        running_ = true;
        matchThread_ = std::thread(&MatchMaker::matchLoop, this);
    }
}

void MatchMaker::stop() {
    if (running_) {
        running_ = false;
        if (matchThread_.joinable()) {
            matchThread_.join();
        }
        queue_.clear();
    }
}

void MatchMaker::addPlayer(const PlayerPtr& player) {
    queue_.addPlayer(player);
}

void MatchMaker::removePlayer(Player::PlayerId playerId) {
    queue_.removePlayer(playerId);
}

RoomPtr MatchMaker::createRoom(const std::vector<PlayerPtr>& players) {
    std::lock_guard<std::mutex> lock(roomsMutex_);
    auto roomId = nextRoomId_++;
    auto room = std::make_shared<Room>(roomId, players.size());
    
    for (const auto& player : players) {
        room->addPlayer(player);
    }
    
    rooms_[roomId] = room;
    return room;
}

std::vector<RoomPtr> MatchMaker::getRooms() const {
    std::lock_guard<std::mutex> lock(roomsMutex_);
    std::vector<RoomPtr> result;
    result.reserve(rooms_.size());
    
    for (const auto& pair : rooms_) {
        result.push_back(pair.second);
    }
    
    return result;
}

void MatchMaker::setMatchStrategy(std::shared_ptr<MatchStrategy> strategy) {
    queue_.setMatchStrategy(strategy);
}

size_t MatchMaker::getQueueSize() const {
    return queue_.size();
}

void MatchMaker::matchLoop() {
    while (running_) {
        std::vector<PlayerPtr> matchedPlayers;
        
        if (queue_.tryMatchPlayers(matchedPlayers, playersPerRoom_, forceMatchOnTimeout_, matchTimeoutThreshold_)) {
            auto room = createRoom(matchedPlayers);
            
            // 记录匹配成功的信息
            std::string playerNames;
            for (size_t i = 0; i < matchedPlayers.size(); ++i) {
                if (i > 0) playerNames += ", ";
                playerNames += matchedPlayers[i]->getName() + "(" + 
                               std::to_string(matchedPlayers[i]->getRating()) + ")";
            }
            
            std::cout << "Match found! Room " << room->getId() 
                      << " with players: " << playerNames << std::endl;
            
            // 触发回调
            if (matchNotifyCallback_) {
                try {
                    matchNotifyCallback_(room);
                } catch (const std::exception& e) {
                    std::cerr << "Exception in match notify callback: " << e.what() << std::endl;
                } catch (...) {
                    std::cerr << "Unknown exception in match notify callback" << std::endl;
                }
            }
        }
        
        // 避免CPU过度使用
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

} // namespace gmatch 