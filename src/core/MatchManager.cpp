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
    
    // 首先获取并检查玩家是否存在
    {
        std::lock_guard<std::mutex> lock(playersMutex_);
        auto it = players_.find(playerId);
        if (it != players_.end()) {
            player = it->second;
            playerExists = true;
        } else {
            LOG_WARNING("Player %llu not found when trying to remove", playerId);
            return;
        }
    }
    
    // 如果玩家在队列中，尝试从队列中移除
    bool wasInQueue = false;
    if (playerExists && player && player->isInQueue() && matchMaker_) {
        try {
            wasInQueue = true;
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
        player->setStatus(false);
    }
    
    // 最后从玩家映射中移除
    {
        std::lock_guard<std::mutex> lock(playersMutex_);
        players_.erase(playerId);
        LOG_DEBUG("Removed player %llu from player list", playerId);
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

} // namespace gmatch 