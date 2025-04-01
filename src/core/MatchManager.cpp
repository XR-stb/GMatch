#include "MatchManager.h"
#include <chrono>

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
    // 获取玩家对象，确保它存在
    PlayerPtr player;
    {
        std::lock_guard<std::mutex> lock(playersMutex_);
        auto it = players_.find(playerId);
        if (it == players_.end()) {
            // 玩家不存在，无需处理
            return;
        }
        player = it->second;
    }
    
    // 先尝试从匹配队列移除
    if (matchMaker_ && player->isInQueue()) {
        matchMaker_->removePlayer(playerId);
        
        // 确保玩家状态正确
        player->setStatus(false);
        
        // 触发回调
        if (playerStatusCallback_) {
            playerStatusCallback_(playerId, false);
        }
    }
    
    // 然后从玩家列表移除
    {
        std::lock_guard<std::mutex> lock(playersMutex_);
        players_.erase(playerId);
    }
}

bool MatchManager::joinMatchmaking(Player::PlayerId playerId) {
    PlayerPtr player = getPlayer(playerId);
    if (!player || !matchMaker_) {
        return false;
    }
    
    // 玩家已经在队列中
    if (player->isInQueue()) {
        return false;
    }
    
    // 更新玩家活动时间
    uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    player->updateActivity(now);
    
    // 添加到匹配队列
    matchMaker_->addPlayer(player);
    
    // 触发回调
    if (playerStatusCallback_) {
        playerStatusCallback_(playerId, true);
    }
    
    return true;
}

bool MatchManager::leaveMatchmaking(Player::PlayerId playerId) {
    PlayerPtr player = getPlayer(playerId);
    if (!player || !matchMaker_) {
        return false;
    }
    
    // 玩家不在队列中
    if (!player->isInQueue()) {
        return false;
    }
    
    // 更新玩家活动时间
    uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    player->updateActivity(now);
    
    // 从匹配队列移除
    matchMaker_->removePlayer(playerId);
    
    // 触发回调
    if (playerStatusCallback_) {
        playerStatusCallback_(playerId, false);
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