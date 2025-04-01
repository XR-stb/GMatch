#include "MatchMaker.h"
#include <algorithm>
#include <cmath>
#include <thread>
#include <chrono>

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
    player->setStatus(true);
    queue_.push_back(player);
}

void MatchQueue::removePlayer(Player::PlayerId playerId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find_if(queue_.begin(), queue_.end(),
        [playerId](const PlayerPtr& player) {
            return player->getId() == playerId;
        });
    
    if (it != queue_.end()) {
        (*it)->setStatus(false);
        queue_.erase(it);
    }
}

bool MatchQueue::tryMatchPlayers(std::vector<PlayerPtr>& matchedPlayers, int requiredPlayers) {
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
        
        if (queue_.tryMatchPlayers(matchedPlayers, playersPerRoom_)) {
            createRoom(matchedPlayers);
        }
        
        // 避免CPU过度使用
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

} // namespace gmatch 