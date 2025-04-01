#include "TcpServer.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include "../util/Logger.h"

namespace gmatch {

// TcpConnection实现
TcpConnection::TcpConnection(int socketFd, ConnectionId id)
    : socketFd_(socketFd), id_(id) {
}

TcpConnection::~TcpConnection() {
    disconnect();
}

bool TcpConnection::send(const std::string& message) {
    if (!connected_) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(writeMutex_);
    
    size_t totalSent = 0;
    size_t messageSize = message.size();
    const char* buffer = message.c_str();
    
    while (totalSent < messageSize) {
        ssize_t sent = ::send(socketFd_, buffer + totalSent, messageSize - totalSent, 0);
        if (sent <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 缓冲区已满，稍后重试
                continue;
            }
            connected_ = false;
            return false;
        }
        
        totalSent += sent;
    }
    
    return true;
}

void TcpConnection::disconnect() {
    bool wasConnected = connected_.exchange(false);
    if (wasConnected) {
        close(socketFd_);
        
        if (readThread_.joinable()) {
            readThread_.join();
        }
        
        // 只在之前是连接状态的情况下调用回调，避免重复调用
        if (disconnectCallback_) {
            disconnectCallback_(id_);
        }
    }
}

void TcpConnection::startReading() {
    readThread_ = std::thread(&TcpConnection::readLoop, this);
}

void TcpConnection::readLoop() {
    const size_t bufferSize = 4096;
    char buffer[bufferSize];
    
    while (connected_) {
        std::memset(buffer, 0, bufferSize);
        ssize_t bytesRead = recv(socketFd_, buffer, bufferSize - 1, 0);
        
        if (bytesRead > 0) {
            if (messageCallback_) {
                messageCallback_(id_, std::string(buffer, bytesRead));
            }
        } else if (bytesRead == 0) {
            // 客户端关闭连接
            break;
        } else {
            // 错误
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                break;
            }
        }
    }
    
    // 确保我们只调用一次disconnect回调
    bool expected = true;
    if (connected_.compare_exchange_strong(expected, false)) {
        close(socketFd_);
        if (disconnectCallback_) {
            disconnectCallback_(id_);
        }
    }
}

// TcpServer实现
TcpServer::TcpServer(const std::string& address, uint16_t port)
    : address_(address), port_(port) {
}

TcpServer::~TcpServer() {
    stop();
}

bool TcpServer::start() {
    if (running_) {
        return true;
    }
    
    // 创建服务器socket
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        LOG_ERROR("Failed to create server socket: %s", strerror(errno));
        return false;
    }
    
    // 设置socket选项
    int opt = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        LOG_ERROR("Failed to set SO_REUSEADDR: %s", strerror(errno));
        close(serverSocket_);
        return false;
    }
    
    // 绑定地址和端口
    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port_);
    
    if (inet_pton(AF_INET, address_.c_str(), &serverAddr.sin_addr) <= 0) {
        LOG_ERROR("Invalid address: %s", address_.c_str());
        close(serverSocket_);
        return false;
    }
    
    if (bind(serverSocket_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        LOG_ERROR("Failed to bind socket: %s", strerror(errno));
        close(serverSocket_);
        return false;
    }
    
    // 监听连接
    if (listen(serverSocket_, SOMAXCONN) < 0) {
        LOG_ERROR("Failed to listen: %s", strerror(errno));
        close(serverSocket_);
        return false;
    }
    
    running_ = true;
    acceptThread_ = std::thread(&TcpServer::acceptLoop, this);
    
    LOG_INFO("Server started at %s:%d", address_.c_str(), port_);
    return true;
}

void TcpServer::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    // 关闭服务器socket
    if (serverSocket_ >= 0) {
        close(serverSocket_);
        serverSocket_ = -1;
    }
    
    // 等待接受线程结束
    if (acceptThread_.joinable()) {
        acceptThread_.join();
    }
    
    // 关闭所有客户端连接
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    for (auto& pair : connections_) {
        pair.second->disconnect();
    }
    connections_.clear();
    
    LOG_INFO("Server stopped");
}

bool TcpServer::sendToClient(TcpConnection::ConnectionId clientId, const std::string& message) {
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    auto it = connections_.find(clientId);
    if (it != connections_.end() && it->second->isConnected()) {
        return it->second->send(message);
    }
    return false;
}

void TcpServer::broadcastMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    for (auto& pair : connections_) {
        if (pair.second->isConnected()) {
            pair.second->send(message);
        }
    }
}

void TcpServer::acceptLoop() {
    while (running_) {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        
        int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK && running_) {
                LOG_ERROR("Failed to accept: %s", strerror(errno));
            }
            continue;
        }
        
        handleNewConnection(clientSocket);
    }
}

void TcpServer::handleNewConnection(int clientSocket) {
    auto clientId = nextClientId_++;
    
    auto connection = std::make_shared<TcpConnection>(clientSocket, clientId);
    connection->setMessageCallback(
        [this](TcpConnection::ConnectionId id, const std::string& msg) {
            handleClientMessage(id, msg);
        });
    connection->setDisconnectCallback(
        [this](TcpConnection::ConnectionId id) {
            handleClientDisconnect(id);
        });
    
    {
        std::lock_guard<std::mutex> lock(connectionsMutex_);
        connections_[clientId] = connection;
    }
    
    if (connectionCallback_) {
        connectionCallback_(connection);
    }
    
    connection->startReading();
}

void TcpServer::handleClientMessage(TcpConnection::ConnectionId clientId, const std::string& message) {
    TcpConnectionPtr connection;
    
    {
        std::lock_guard<std::mutex> lock(connectionsMutex_);
        auto it = connections_.find(clientId);
        if (it != connections_.end()) {
            connection = it->second;
        }
    }
    
    if (connection && messageCallback_) {
        messageCallback_(connection, message);
    }
}

void TcpServer::handleClientDisconnect(TcpConnection::ConnectionId clientId) {
    TcpConnectionPtr connection;
    
    {
        std::lock_guard<std::mutex> lock(connectionsMutex_);
        auto it = connections_.find(clientId);
        if (it != connections_.end()) {
            connection = it->second;
            connections_.erase(it);
        }
    }
    
    if (connection && closeCallback_) {
        closeCallback_(connection);
    }
}

} // namespace gmatch 