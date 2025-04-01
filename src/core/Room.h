#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <memory>
#include <chrono>
#include "Player.h"

namespace gmatch {

class Room {
public:
    using RoomId = uint64_t;
    enum class Status {
        WAITING,  // 等待玩家加入
        READY,    // 玩家已满，准备开始
        STARTED,  // 已开始
        FINISHED  // 已结束
    };

    Room(RoomId id, int capacity, int minRating = 0, int maxRating = 0);
    ~Room() = default;

    RoomId getId() const { return id_; }
    Status getStatus() const { return status_; }
    int getCapacity() const { return capacity_; }
    int getPlayerCount() const { return static_cast<int>(players_.size()); }
    bool isFull() const { return players_.size() >= capacity_; }
    
    bool addPlayer(const PlayerPtr& player);
    bool removePlayer(Player::PlayerId playerId);
    void setStatus(Status status);
    
    uint64_t getCreationTime() const { return creationTime_; }
    std::vector<PlayerPtr> getPlayers() const;
    
    bool isRatingInRange(int rating) const;
    double getAverageRating() const;

private:
    RoomId id_;
    Status status_ = Status::WAITING;
    int capacity_;
    int minRating_;  // 最小允许评分，0表示不限制
    int maxRating_;  // 最大允许评分，0表示不限制
    std::unordered_map<Player::PlayerId, PlayerPtr> players_;
    uint64_t creationTime_;
};

using RoomPtr = std::shared_ptr<Room>;

} // namespace gmatch 