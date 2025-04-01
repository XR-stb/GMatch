#pragma once

#include <string>
#include <functional>
#include <unordered_map>
#include "TcpServer.h"
#include "../core/MatchManager.h"

namespace gmatch {

// 请求处理器接口
class RequestHandler {
public:
    virtual ~RequestHandler() = default;
    virtual std::string handleRequest(const std::string& request, TcpConnection::ConnectionId clientId) = 0;
};

// JSON请求处理器
class JsonRequestHandler : public RequestHandler {
public:
    using CommandHandler = std::function<std::string(const std::string&, TcpConnection::ConnectionId)>;
    using PlayerCreatedCallback = std::function<void(TcpConnection::ConnectionId, Player::PlayerId)>;
    
    JsonRequestHandler();
    
    // 处理请求
    std::string handleRequest(const std::string& request, TcpConnection::ConnectionId clientId) override;
    
    // 注册命令处理器
    void registerCommandHandler(const std::string& command, CommandHandler handler);
    
    // 设置玩家创建回调
    void setPlayerCreatedCallback(PlayerCreatedCallback callback) {
        onPlayerCreatedCallback_ = callback;
    }
    
private:
    // 解析JSON请求
    bool parseJsonRequest(const std::string& request, std::string& command, std::string& data);
    
    // 构造JSON响应
    std::string createJsonResponse(const std::string& command, bool success, const std::string& message, const std::string& data = "");
    
    // 命令处理映射表
    std::unordered_map<std::string, CommandHandler> commandHandlers_;
    
    // 玩家创建回调
    PlayerCreatedCallback onPlayerCreatedCallback_;
    
    // 默认命令处理方法
    std::string handleCreatePlayer(const std::string& data, TcpConnection::ConnectionId clientId);
    std::string handleJoinMatchmaking(const std::string& data, TcpConnection::ConnectionId clientId);
    std::string handleLeaveMatchmaking(const std::string& data, TcpConnection::ConnectionId clientId);
    std::string handleGetRooms(const std::string& data, TcpConnection::ConnectionId clientId);
    std::string handleGetPlayerInfo(const std::string& data, TcpConnection::ConnectionId clientId);
    std::string handleGetQueueStatus(const std::string& data, TcpConnection::ConnectionId clientId);
};

} // namespace gmatch 