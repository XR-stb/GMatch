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

namespace gmatch {

// 匹配策略接口
class MatchStrategy {
public:
    virtual ~MatchStrategy() = default;
    virtual bool isMatch(const PlayerPtr& player1, const PlayerPtr& player2) const = 0;
};

// 基于评分差异的匹配策略
class RatingBasedStrategy : public MatchStrategy {
public:
    RatingBasedStrategy(int maxRatingDiff = 300);
    bool isMatch(const PlayerPtr& player1, const PlayerPtr& player2) const override;
    
private:
    int maxRatingDiff_;
};

// 匹配队列
class MatchQueue {
public:
    MatchQueue();
    void addPlayer(const PlayerPtr& player);
    void removePlayer(Player::PlayerId playerId);
    bool tryMatchPlayers(std::vector<PlayerPtr>& matchedPlayers, int requiredPlayers);
    size_t size() const;
    
    void setMatchStrategy(std::shared_ptr<MatchStrategy> strategy);
    void clear();
    
private:
    std::vector<PlayerPtr> queue_;
    std::shared_ptr<MatchStrategy> matchStrategy_;
    mutable std::mutex mutex_;
};

// 匹配器
class MatchMaker {
public:
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
    
    // 获取当前队列大小
    size_t getQueueSize() const;
    
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
};

} // namespace gmatch 