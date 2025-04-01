#include "MatchMaker.h"
#include <algorithm>
#include <cmath>
#include <thread>
#include <chrono>
#include <iostream>

namespace gmatch {

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
        
        if (queue_.tryMatchPlayers(matchedPlayers, playersPerRoom_, forceMatchOnTimeout_, matchTimeoutThreshold_)) {
            auto room = createRoom(matchedPlayers);
            
            // 记录匹配成功的信息
            std::string playerNames;
            for (size_t i = 0; i < matchedPlayers.size(); ++i) {
                if (i > 0) playerNames += ", ";
                playerNames += matchedPlayers[i]->getName() + "(" + 
                               std::to_string(matchedPlayers[i]->getRating()) + ")";
            }
            
            std::cout << "Match found! Room " << room->getId() 
                      << " with players: " << playerNames << std::endl;
            
            // 触发回调
            if (matchNotifyCallback_) {
                try {
                    matchNotifyCallback_(room);
                } catch (const std::exception& e) {
                    std::cerr << "Exception in match notify callback: " << e.what() << std::endl;
                } catch (...) {
                    std::cerr << "Unknown exception in match notify callback" << std::endl;
                }
            }
        }
        
        // 避免CPU过度使用
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

} // namespace gmatch 