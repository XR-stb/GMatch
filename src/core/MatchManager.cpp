#include "MatchManager.h"
#include <chrono>
#include <stdexcept>
#include "../util/Logger.h"

namespace gmatch {

MatchManager::MatchManager() {
}

MatchManager::~MatchManager() {
    shutdown();
}

MatchManager& MatchManager::getInstance() {
    static MatchManager instance;
    return instance;
}

void MatchManager::init(int playersPerRoom) {
    if (!initialized_) {
        matchMaker_ = std::make_shared<MatchMaker>(playersPerRoom);
        
        // 设置默认策略
        auto strategy = std::make_shared<RatingBasedStrategy>(300);
        matchMaker_->setMatchStrategy(strategy);
        
        // 设置匹配通知回调
        matchMaker_->setMatchNotifyCallback([this](const RoomPtr& room) {
            if (matchNotifyCallback_) {
                try {
                    LOG_DEBUG("Room %llu created, notifying callback", room->getId());
                    matchNotifyCallback_(room);
                } catch (const std::exception& e) {
                    LOG_ERROR("Exception in match notify callback: %s", e.what());
                } catch (...) {
                    LOG_ERROR("Unknown exception in match notify callback");
                }
            }
        });
        
        // 启动匹配线程
        matchMaker_->start();
        initialized_ = true;
    }
}

void MatchManager::shutdown() {
    if (initialized_) {
        if (matchMaker_) {
            matchMaker_->stop();
        }
        
        std::lock_guard<std::mutex> lock(playersMutex_);
        players_.clear();
        initialized_ = false;
    }
}

PlayerPtr MatchManager::createPlayer(const std::string& name, int rating) {
    std::lock_guard<std::mutex> lock(playersMutex_);
    auto playerId = nextPlayerId_++;
    auto player = std::make_shared<Player>(playerId, name, rating);
    players_[playerId] = player;
    
    // 更新玩家活动时间
    uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    player->updateActivity(now);
    
    return player;
}

PlayerPtr MatchManager::getPlayer(Player::PlayerId playerId) {
    std::lock_guard<std::mutex> lock(playersMutex_);
    auto it = players_.find(playerId);
    if (it != players_.end()) {
        return it->second;
    }
    return nullptr;
}

void MatchManager::removePlayer(Player::PlayerId playerId) {
    if (!initialized_) {
        LOG_WARNING("Trying to remove player %llu but MatchManager is not initialized", playerId);
        return;
    }
    
    PlayerPtr player;
    bool playerExists = false;
    bool wasInQueue = false;
    
    // 首先获取并检查玩家是否存在
    try {
        std::lock_guard<std::mutex> lock(playersMutex_);
        auto it = players_.find(playerId);
        if (it != players_.end()) {
            player = it->second;
            playerExists = true;
            // 记录玩家是否在队列中，避免后续获取时可能发生的竞态
            wasInQueue = player->isInQueue();
            
            // 在持有锁的情况下直接移除玩家，避免后续操作中玩家被其他线程修改
            players_.erase(it);
            LOG_DEBUG("Removed player %llu from player list", playerId);
        } else {
            LOG_WARNING("Player %llu not found when trying to remove", playerId);
            return;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Exception accessing player %llu: %s", playerId, e.what());
        return;
    } catch (...) {
        LOG_ERROR("Unknown exception accessing player %llu", playerId);
        return;
    }
    
    // 如果玩家在队列中，尝试从队列中移除
    if (playerExists && wasInQueue && matchMaker_) {
        try {
            // 从匹配队列移除，但不改变玩家状态
            matchMaker_->removePlayer(playerId);
            LOG_DEBUG("Removed player %llu from queue", playerId);
        } catch (const std::exception& e) {
            LOG_ERROR("Exception removing player %llu from queue: %s", playerId, e.what());
        } catch (...) {
            LOG_ERROR("Unknown exception removing player %llu from queue", playerId);
        }
    }
    
    // 手动更新玩家状态，这应该在MatchMaker::removePlayer之后设置
    if (playerExists && player && wasInQueue) {
        try {
            player->setStatus(false);
        } catch (const std::exception& e) {
            LOG_ERROR("Exception setting player %llu status: %s", playerId, e.what());
        } catch (...) {
            LOG_ERROR("Unknown exception setting player %llu status", playerId);
        }
    }
    
    // 如果玩家原本在队列中，触发状态变更回调
    if (wasInQueue && playerStatusCallback_) {
        try {
            playerStatusCallback_(playerId, false);
            LOG_DEBUG("Triggered status callback for player %llu", playerId);
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in player status callback for %llu: %s", playerId, e.what());
        } catch (...) {
            LOG_ERROR("Unknown exception in player status callback for %llu", playerId);
        }
    }
}

bool MatchManager::joinMatchmaking(Player::PlayerId playerId) {
    PlayerPtr player = getPlayer(playerId);
    if (!player || !matchMaker_) {
        LOG_WARNING("Player %llu not found or matchmaker not initialized", playerId);
        return false;
    }
    
    // 玩家已经在队列中
    if (player->isInQueue()) {
        LOG_DEBUG("Player %llu is already in queue", playerId);
        return false;
    }
    
    // 更新玩家活动时间
    uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    player->updateActivity(now);
    
    LOG_DEBUG("Adding player %llu to matchmaking queue", playerId);
    
    // 首先更新玩家状态
    player->setStatus(true);
    
    // 添加到匹配队列
    try {
        matchMaker_->addPlayer(player);
        LOG_DEBUG("Player %llu added to queue", playerId);
    } catch (const std::exception& e) {
        LOG_ERROR("Exception adding player %llu to queue: %s", playerId, e.what());
        player->setStatus(false);  // 如果添加失败，恢复状态
        return false;
    } catch (...) {
        LOG_ERROR("Unknown exception adding player %llu to queue", playerId);
        player->setStatus(false);  // 如果添加失败，恢复状态
        return false;
    }
    
    // 触发回调
    if (playerStatusCallback_) {
        try {
            LOG_DEBUG("Triggering player status callback for %llu", playerId);
            playerStatusCallback_(playerId, true);
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in player status callback: %s", e.what());
        } catch (...) {
            LOG_ERROR("Unknown exception in player status callback");
        }
    }
    
    return true;
}

bool MatchManager::leaveMatchmaking(Player::PlayerId playerId) {
    PlayerPtr player = getPlayer(playerId);
    if (!player || !matchMaker_) {
        LOG_WARNING("Player %llu not found or matchmaker not initialized", playerId);
        return false;
    }
    
    // 玩家不在队列中
    if (!player->isInQueue()) {
        LOG_DEBUG("Player %llu is not in queue", playerId);
        return false;
    }
    
    // 更新玩家活动时间
    uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    player->updateActivity(now);
    
    LOG_DEBUG("Removing player %llu from matchmaking queue", playerId);
    
    // 从匹配队列移除（不修改玩家状态）
    matchMaker_->removePlayer(playerId);
    
    // 手动更新玩家状态
    player->setStatus(false);
    LOG_DEBUG("Player %llu status updated to not in queue", playerId);
    
    // 触发回调
    if (playerStatusCallback_) {
        try {
            LOG_DEBUG("Triggering player status callback for %llu", playerId);
            playerStatusCallback_(playerId, false);
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in player status callback: %s", e.what());
        } catch (...) {
            LOG_ERROR("Unknown exception in player status callback");
        }
    }
    
    return true;
}

RoomPtr MatchManager::getRoom(Room::RoomId roomId) {
    if (!matchMaker_) {
        return nullptr;
    }
    
    auto rooms = matchMaker_->getRooms();
    for (const auto& room : rooms) {
        if (room->getId() == roomId) {
            return room;
        }
    }
    
    return nullptr;
}

std::vector<RoomPtr> MatchManager::getAllRooms() const {
    if (!matchMaker_) {
        return {};
    }
    
    return matchMaker_->getRooms();
}

void MatchManager::setMatchNotifyCallback(MatchNotifyCallback callback) {
    matchNotifyCallback_ = callback;
}

void MatchManager::setPlayerStatusCallback(PlayerStatusCallback callback) {
    playerStatusCallback_ = callback;
}

void MatchManager::setMaxRatingDifference(int maxDiff) {
    if (matchMaker_) {
        auto strategy = std::make_shared<RatingBasedStrategy>(maxDiff);
        matchMaker_->setMatchStrategy(strategy);
    }
}

size_t MatchManager::getQueueSize() const {
    if (!matchMaker_) {
        return 0;
    }
    
    return matchMaker_->getQueueSize();
}

size_t MatchManager::getPlayerCount() const {
    std::lock_guard<std::mutex> lock(playersMutex_);
    return players_.size();
}

size_t MatchManager::getRoomCount() const {
    if (!matchMaker_) {
        return 0;
    }
    
    return matchMaker_->getRooms().size();
}

void MatchManager::setForceMatchOnTimeout(bool enable) {
    if (matchMaker_) {
        matchMaker_->setForceMatchOnTimeout(enable);
    }
}

void MatchManager::setMatchTimeoutThreshold(uint64_t ms) {
    if (matchMaker_) {
        matchMaker_->setMatchTimeoutThreshold(ms);
    }
}

bool MatchManager::getForceMatchOnTimeout() const {
    if (matchMaker_) {
        return matchMaker_->getForceMatchOnTimeout();
    }
    return false;
}

void MatchManager::printMatchmakingStatus(std::ostream& out) const {
    if (!initialized_ || !matchMaker_) {
        out << "Matchmaking system not initialized" << std::endl;
        return;
    }
    
    // 基本信息
    out << "\n==== Matchmaking Status ====\n";
    
    // 获取队列中的玩家
    std::vector<PlayerPtr> queuedPlayers;
    {
        std::lock_guard<std::mutex> lock(playersMutex_);
        for (const auto& pair : players_) {
            if (pair.second->isInQueue()) {
                queuedPlayers.push_back(pair.second);
            }
        }
    }
    
    // 按照评分排序
    std::sort(queuedPlayers.begin(), queuedPlayers.end(),
        [](const PlayerPtr& a, const PlayerPtr& b) {
            return a->getRating() < b->getRating();
        });
    
    // 输出队列状态
    out << "Queue: " << queuedPlayers.size() << " players waiting\n";
    if (!queuedPlayers.empty()) {
        out << "  ID  | Name             | Rating | Wait Time (ms)\n";
        out << "------+------------------+--------+--------------\n";
        
        uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
            
        for (const auto& player : queuedPlayers) {
            uint64_t waitTime = now - player->getLastActivityTime();
            
            char idStr[10], ratingStr[10], waitTimeStr[15];
            snprintf(idStr, sizeof(idStr), "%5lu", player->getId());
            snprintf(ratingStr, sizeof(ratingStr), "%7d", player->getRating());
            snprintf(waitTimeStr, sizeof(waitTimeStr), "%12lu", waitTime);
            
            out << "  " << idStr << " | ";
            
            // 限制名称长度为16个字符
            std::string name = player->getName();
            if (name.length() > 16) {
                name = name.substr(0, 13) + "...";
            } else {
                name.append(16 - name.length(), ' ');
            }
            out << name << " | " << ratingStr << " | " << waitTimeStr << "\n";
        }
    }
    
    // 获取活跃房间
    auto rooms = matchMaker_->getRooms();
    out << "\nActive Rooms: " << rooms.size() << "\n";
    
    if (!rooms.empty()) {
        out << "  Room ID | Players\n";
        out << "---------+----------------------------------\n";
        
        for (const auto& room : rooms) {
            auto players = room->getPlayers();
            
            char roomIdStr[10];
            snprintf(roomIdStr, sizeof(roomIdStr), "%8lu", room->getId());
            
            out << "  " << roomIdStr << " | ";
            
            for (size_t i = 0; i < players.size(); ++i) {
                if (i > 0) out << ", ";
                out << players[i]->getName() << " (" << players[i]->getRating() << ")";
            }
            out << "\n";
        }
    }
    
    // 匹配配置信息
    auto matchStrategy = matchMaker_->getMatchStrategy();
    auto strategy = dynamic_cast<RatingBasedStrategy*>(matchStrategy.get());
    int maxRatingDiff = strategy ? strategy->getMaxRatingDiff() : 0;
    
    out << "\nMatchmaking Config:\n";
    out << "  Players per Room: " << matchMaker_->playersPerRoom_ << "\n";
    out << "  Max Rating Diff: " << maxRatingDiff << "\n";
    out << "  Force Match on Timeout: " << (matchMaker_->getForceMatchOnTimeout() ? "Yes" : "No") << "\n";
    out << "  Match Timeout Threshold: " << matchMaker_->getMatchTimeoutThreshold() << "ms\n";
    
    out << "============================\n" << std::endl;
}

} // namespace gmatch 