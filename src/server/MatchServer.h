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
    
    // 设置是否启用超时强制匹配
    void setForceMatchOnTimeout(bool enable);
    
    // 设置超时强制匹配的阈值(毫秒)
    void setMatchTimeoutThreshold(uint64_t ms);
    
    // 输出当前匹配系统状态
    void printMatchmakingStatus(std::ostream& out = std::cout) const;
    
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