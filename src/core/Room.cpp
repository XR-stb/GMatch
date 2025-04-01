#include "Room.h"
#include <algorithm>
#include <numeric>

namespace gmatch {

Room::Room(RoomId id, int capacity, int minRating, int maxRating)
    : id_(id), capacity_(capacity), minRating_(minRating), maxRating_(maxRating) {
    creationTime_ = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

bool Room::addPlayer(const PlayerPtr& player) {
    if (status_ != Status::WAITING || isFull()) {
        return false;
    }
    
    if (!isRatingInRange(player->getRating())) {
        return false;
    }
    
    auto playerId = player->getId();
    auto result = players_.emplace(playerId, player);
    
    if (isFull()) {
        status_ = Status::READY;
    }
    
    return result.second;
}

bool Room::removePlayer(Player::PlayerId playerId) {
    auto it = players_.find(playerId);
    if (it != players_.end()) {
        players_.erase(it);
        if (status_ == Status::READY) {
            status_ = Status::WAITING;
        }
        return true;
    }
    return false;
}

void Room::setStatus(Status status) {
    status_ = status;
}

std::vector<PlayerPtr> Room::getPlayers() const {
    std::vector<PlayerPtr> result;
    result.reserve(players_.size());
    for (const auto& pair : players_) {
        result.push_back(pair.second);
    }
    return result;
}

bool Room::isRatingInRange(int rating) const {
    if (minRating_ > 0 && rating < minRating_) {
        return false;
    }
    if (maxRating_ > 0 && rating > maxRating_) {
        return false;
    }
    return true;
}

double Room::getAverageRating() const {
    if (players_.empty()) {
        return 0.0;
    }
    
    int sum = std::accumulate(players_.begin(), players_.end(), 0,
        [](int total, const auto& pair) {
            return total + pair.second->getRating();
        });
    
    return static_cast<double>(sum) / players_.size();
}

} // namespace gmatch 