#pragma once

#include <memory>
#include <functional>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include "Player.h"
#include "MatchMaker.h"

namespace gmatch {

// 匹配通知回调类型
using MatchNotifyCallback = std::function<void(const RoomPtr&)>;
using PlayerStatusCallback = std::function<void(Player::PlayerId, bool)>;

// 匹配管理器（单例）
class MatchManager {
public:
    static MatchManager& getInstance();
    
    // 禁止拷贝和移动
    MatchManager(const MatchManager&) = delete;
    MatchManager& operator=(const MatchManager&) = delete;
    MatchManager(MatchManager&&) = delete;
    MatchManager& operator=(MatchManager&&) = delete;
    
    // 初始化和关闭
    void init(int playersPerRoom = 2);
    void shutdown();
    
    // 玩家管理
    PlayerPtr createPlayer(const std::string& name, int rating = 1500);
    PlayerPtr getPlayer(Player::PlayerId playerId);
    void removePlayer(Player::PlayerId playerId);
    
    // 匹配控制
    bool joinMatchmaking(Player::PlayerId playerId);
    bool leaveMatchmaking(Player::PlayerId playerId);
    
    // 房间管理
    RoomPtr getRoom(Room::RoomId roomId);
    std::vector<RoomPtr> getAllRooms() const;
    
    // 回调注册
    void setMatchNotifyCallback(MatchNotifyCallback callback);
    void setPlayerStatusCallback(PlayerStatusCallback callback);
    
    // 匹配策略设置
    void setMaxRatingDifference(int maxDiff);
    
    // 高级功能
    size_t getQueueSize() const;
    size_t getPlayerCount() const;
    size_t getRoomCount() const;
    
private:
    MatchManager();
    ~MatchManager();
    
    std::shared_ptr<MatchMaker> matchMaker_;
    std::unordered_map<Player::PlayerId, PlayerPtr> players_;
    std::atomic<Player::PlayerId> nextPlayerId_{1};
    mutable std::mutex playersMutex_;
    
    MatchNotifyCallback matchNotifyCallback_;
    PlayerStatusCallback playerStatusCallback_;
    bool initialized_ = false;
};

} // namespace gmatch 