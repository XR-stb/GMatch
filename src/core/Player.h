#pragma once

#include <string>
#include <cstdint>
#include <memory>

namespace gmatch {

class Player {
public:
    using PlayerId = uint64_t;
    
    Player(PlayerId id, const std::string& name, int rating = 1500);
    ~Player() = default;

    PlayerId getId() const { return id_; }
    const std::string& getName() const { return name_; }
    int getRating() const { return rating_; }
    void setRating(int rating) { rating_ = rating; }
    
    void setStatus(bool isInQueue) { isInQueue_ = isInQueue; }
    bool isInQueue() const { return isInQueue_; }
    
    uint64_t getLastActivityTime() const { return lastActivityTime_; }
    void updateActivity(uint64_t timestamp) { lastActivityTime_ = timestamp; }

private:
    PlayerId id_;
    std::string name_;
    int rating_;
    bool isInQueue_ = false;
    uint64_t lastActivityTime_ = 0;
};

using PlayerPtr = std::shared_ptr<Player>;

} // namespace gmatch 