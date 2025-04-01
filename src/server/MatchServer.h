#pragma once

#include <memory>
#include <unordered_map>
#include "TcpServer.h"
#include "RequestHandler.h"
#include "../core/MatchManager.h"
#include "../util/Logger.h"

namespace gmatch {

class MatchServer {
public:
    MatchServer(const std::string& address = "0.0.0.0", uint16_t port = 8080);
    ~MatchServer();
    
    bool start();
    void stop();
    
    void setPlayersPerRoom(int playersPerRoom);
    void setMaxRatingDifference(int maxDiff);
    
    void setLogLevel(LogLevel level);
    void setLogFile(const std::string& filename);
    
    bool isRunning() const;
    
private:
    void onClientConnected(const TcpConnectionPtr& conn);
    void onClientMessage(const TcpConnectionPtr& conn, const std::string& message);
    void onClientDisconnected(const TcpConnectionPtr& conn);
    
    void onMatchNotify(const RoomPtr& room);
    void onPlayerStatusChanged(Player::PlayerId playerId, bool inQueue);
    
    std::unique_ptr<TcpServer> server_;
    std::unique_ptr<JsonRequestHandler> requestHandler_;
    
    std::unordered_map<TcpConnection::ConnectionId, Player::PlayerId> clientPlayerMap_;
    std::mutex clientMapMutex_;
    
    bool initialized_ = false;
};

} // namespace gmatch 