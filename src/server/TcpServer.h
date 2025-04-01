#pragma once

#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <unordered_map>
#include <memory>

namespace gmatch {

// 用于表示连接的客户端
class TcpConnection {
public:
    using ConnectionId = uint64_t;
    using MessageCallback = std::function<void(ConnectionId, const std::string&)>;
    using DisconnectCallback = std::function<void(ConnectionId)>;
    
    TcpConnection(int socketFd, ConnectionId id);
    ~TcpConnection();
    
    ConnectionId getId() const { return id_; }
    bool isConnected() const { return connected_; }
    
    bool send(const std::string& message);
    void disconnect();
    void disconnectWithoutCallback();  // 断开连接但不触发回调
    
    void setMessageCallback(MessageCallback callback) { messageCallback_ = callback; }
    void setDisconnectCallback(DisconnectCallback callback) { disconnectCallback_ = callback; }
    
    void startReading();
    
private:
    void readLoop();
    
    int socketFd_;
    ConnectionId id_;
    std::atomic<bool> connected_{true};
    std::thread readThread_;
    std::mutex writeMutex_;
    
    MessageCallback messageCallback_;
    DisconnectCallback disconnectCallback_;
};

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

// TCP服务器
class TcpServer {
public:
    using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
    using MessageCallback = std::function<void(const TcpConnectionPtr&, const std::string&)>;
    using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
    
    TcpServer(const std::string& address = "0.0.0.0", uint16_t port = 8080);
    ~TcpServer();
    
    bool start();
    void stop();
    
    bool isRunning() const { return running_; }
    
    void setConnectionCallback(ConnectionCallback callback) { connectionCallback_ = callback; }
    void setMessageCallback(MessageCallback callback) { messageCallback_ = callback; }
    void setCloseCallback(CloseCallback callback) { closeCallback_ = callback; }
    
    // 向特定客户端发送消息
    bool sendToClient(TcpConnection::ConnectionId clientId, const std::string& message);
    
    // 向所有客户端广播消息
    void broadcastMessage(const std::string& message);
    
private:
    void acceptLoop();
    void handleNewConnection(int clientSocket);
    void handleClientMessage(TcpConnection::ConnectionId clientId, const std::string& message);
    void handleClientDisconnect(TcpConnection::ConnectionId clientId);
    
    std::string address_;
    uint16_t port_;
    int serverSocket_ = -1;
    std::atomic<bool> running_{false};
    std::thread acceptThread_;
    
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    CloseCallback closeCallback_;
    
    std::mutex connectionsMutex_;
    std::unordered_map<TcpConnection::ConnectionId, TcpConnectionPtr> connections_;
    std::atomic<TcpConnection::ConnectionId> nextClientId_{1};
};

} // namespace gmatch 