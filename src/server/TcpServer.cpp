#include "TcpServer.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include "../util/Logger.h"

namespace gmatch {

// TcpServer实现
TcpServer::TcpServer(const std::string& address, uint16_t port)
    : address_(address), port_(port) {
    LOG_DEBUG("Creating TcpServer at %s:%d", address.c_str(), port);
}

TcpServer::~TcpServer() {
    LOG_DEBUG("Destroying TcpServer");
    stop();
}

bool TcpServer::start() {
    if (running_) {
        LOG_DEBUG("TcpServer already running");
        return true;
    }
    
    LOG_DEBUG("Starting TcpServer");
    
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
        LOG_DEBUG("TcpServer already stopped");
        return;
    }
    
    LOG_DEBUG("Stopping TcpServer");
    running_ = false;
    
    // 关闭服务器socket
    if (serverSocket_ >= 0) {
        LOG_DEBUG("Closing server socket");
        close(serverSocket_);
        serverSocket_ = -1;
    }
    
    // 等待接受线程结束
    if (acceptThread_.joinable()) {
        LOG_DEBUG("Joining accept thread");
        acceptThread_.join();
        LOG_DEBUG("Accept thread joined");
    }
    
    // 关闭所有客户端连接，但不触发回调，因为服务器正在关闭
    LOG_DEBUG("Closing all client connections");
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    for (auto& pair : connections_) {
        LOG_DEBUG("Disconnecting client %llu", pair.first);
        pair.second->disconnectWithoutCallback();
    }
    connections_.clear();
    
    LOG_INFO("Server stopped");
}

bool TcpServer::sendToClient(TcpConnection::ConnectionId clientId, const std::string& message) {
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    auto it = connections_.find(clientId);
    if (it != connections_.end() && it->second->isConnected()) {
        LOG_DEBUG("Sending message to client %llu", clientId);
        return it->second->send(message);
    }
    LOG_DEBUG("Client %llu not found or not connected", clientId);
    return false;
}

void TcpServer::broadcastMessage(const std::string& message) {
    LOG_DEBUG("Broadcasting message to all clients");
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    for (auto& pair : connections_) {
        if (pair.second->isConnected()) {
            LOG_DEBUG("Broadcasting to client %llu", pair.first);
            pair.second->send(message);
        }
    }
}

void TcpServer::acceptLoop() {
    LOG_DEBUG("Accept loop started");
    while (running_) {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        
        LOG_DEBUG("Waiting for new connections");
        int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK && running_) {
                LOG_ERROR("Failed to accept: %s", strerror(errno));
            }
            continue;
        }
        
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
        LOG_DEBUG("New connection from %s:%d", clientIP, ntohs(clientAddr.sin_port));
        
        handleNewConnection(clientSocket);
    }
    LOG_DEBUG("Accept loop ended");
}

void TcpServer::handleNewConnection(int clientSocket) {
    auto clientId = nextClientId_++;
    LOG_DEBUG("Handling new connection, assigned ID %llu", clientId);
    
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
        LOG_DEBUG("Added client %llu to connections map", clientId);
    }
    
    if (connectionCallback_) {
        try {
            LOG_DEBUG("Calling connection callback for client %llu", clientId);
            connectionCallback_(connection);
            LOG_DEBUG("Connection callback completed for client %llu", clientId);
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in connection callback for client %llu: %s", clientId, e.what());
        } catch (...) {
            LOG_ERROR("Unknown exception in connection callback for client %llu", clientId);
        }
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
            LOG_DEBUG("Found connection for client %llu", clientId);
        } else {
            LOG_DEBUG("Connection not found for client %llu", clientId);
        }
    }
    
    if (connection && messageCallback_) {
        try {
            LOG_DEBUG("Calling message callback for client %llu", clientId);
            messageCallback_(connection, message);
            LOG_DEBUG("Message callback completed for client %llu", clientId);
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in message callback for client %llu: %s", clientId, e.what());
        } catch (...) {
            LOG_ERROR("Unknown exception in message callback for client %llu", clientId);
        }
    }
}

void TcpServer::handleClientDisconnect(TcpConnection::ConnectionId clientId) {
    LOG_DEBUG("Handling client disconnect for client %llu", clientId);
    TcpConnectionPtr connection;
    
    try {
        {
            std::lock_guard<std::mutex> lock(connectionsMutex_);
            auto it = connections_.find(clientId);
            if (it != connections_.end()) {
                LOG_DEBUG("Found connection for client %llu, removing from map", clientId);
                connection = it->second;
                connections_.erase(it);
            } else {
                LOG_DEBUG("Connection not found for client %llu", clientId);
            }
        }
        
        if (connection && closeCallback_) {
            try {
                LOG_DEBUG("Calling close callback for client %llu", clientId);
                closeCallback_(connection);
                LOG_DEBUG("Close callback completed for client %llu", clientId);
            } catch (const std::exception& e) {
                LOG_ERROR("Exception in close callback for client %llu: %s", clientId, e.what());
            } catch (...) {
                LOG_ERROR("Unknown exception in close callback for client %llu", clientId);
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in handleClientDisconnect for client %llu: %s", clientId, e.what());
    } catch (...) {
        LOG_ERROR("Unknown exception in handleClientDisconnect for client %llu", clientId);
    }
}

} // namespace gmatch 