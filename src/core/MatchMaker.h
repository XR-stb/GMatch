#pragma once

#include <vector>
#include <queue>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include "Player.h"
#include "Room.h"
#include "MatchQueue.h"
#include "MatchStrategy.h"

namespace gmatch {

// 匹配器
class MatchMaker {
public:
    // 匹配通知回调类型
    using MatchNotifyCallback = std::function<void(const RoomPtr&)>;
    
    explicit MatchMaker(int playersPerRoom = 2);
    ~MatchMaker();
    
    void start();
    void stop();
    
    void addPlayer(const PlayerPtr& player);
    void removePlayer(Player::PlayerId playerId);
    
    RoomPtr createRoom(const std::vector<PlayerPtr>& players);
    std::vector<RoomPtr> getRooms() const;
    
    // 设置匹配策略
    void setMatchStrategy(std::shared_ptr<MatchStrategy> strategy);
    
    // 设置匹配通知回调
    void setMatchNotifyCallback(MatchNotifyCallback callback) {
        matchNotifyCallback_ = callback;
    }
    
    // 获取当前队列大小
    size_t getQueueSize() const;
    
    // 设置是否启用超时强制匹配
    void setForceMatchOnTimeout(bool enable) {
        forceMatchOnTimeout_ = enable;
    }
    
    // 设置超时强制匹配的阈值(毫秒)
    void setMatchTimeoutThreshold(uint64_t ms) {
        matchTimeoutThreshold_ = ms;
    }
    
    // 获取超时强制匹配状态
    bool getForceMatchOnTimeout() const {
        return forceMatchOnTimeout_;
    }
    
    // 获取匹配超时阈值
    uint64_t getMatchTimeoutThreshold() const {
        return matchTimeoutThreshold_;
    }
    
    // 获取当前使用的匹配策略
    std::shared_ptr<MatchStrategy> getMatchStrategy() const {
        return queue_.getMatchStrategy();
    }
    
    // 声明友元
    friend class MatchManager;
    
private:
    void matchLoop();
    
    std::unordered_map<Room::RoomId, RoomPtr> rooms_;
    MatchQueue queue_;
    std::atomic<bool> running_{false};
    std::thread matchThread_;
    mutable std::mutex roomsMutex_;
    
    int playersPerRoom_;
    std::atomic<Room::RoomId> nextRoomId_{1};
    MatchNotifyCallback matchNotifyCallback_;
    
    // 超时匹配控制
    std::atomic<bool> forceMatchOnTimeout_{false};
    std::atomic<uint64_t> matchTimeoutThreshold_{5000}; // 默认5秒
};

} // namespace gmatch 