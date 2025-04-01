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

// TcpConnection实现
TcpConnection::TcpConnection(int socketFd, ConnectionId id)
    : socketFd_(socketFd), id_(id) {
    LOG_DEBUG("Creating TcpConnection with ID %llu", id);
}

TcpConnection::~TcpConnection() {
    LOG_DEBUG("Destroying TcpConnection with ID %llu", id_);
    // 断开连接但不触发回调，因为可能正在进行cleanup
    disconnectWithoutCallback();
}

void TcpConnection::disconnectWithoutCallback() {
    bool wasConnected = connected_.exchange(false);
    LOG_DEBUG("Silent disconnecting client %llu (was connected: %d)", id_, wasConnected);
    
    if (wasConnected) {
        try {
            LOG_DEBUG("Closing socket for client %llu", id_);
            close(socketFd_);
            
            if (readThread_.joinable()) {
                LOG_DEBUG("Joining read thread for client %llu", id_);
                readThread_.join();
                LOG_DEBUG("Read thread joined for client %llu", id_);
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in silent disconnect for client %llu: %s", id_, e.what());
        } catch (...) {
            LOG_ERROR("Unknown exception in silent disconnect for client %llu", id_);
        }
    }
}

void TcpConnection::disconnect() {
    bool wasConnected = connected_.exchange(false);
    LOG_DEBUG("Disconnecting client %llu (was connected: %d)", id_, wasConnected);
    
    if (wasConnected) {
        try {
            LOG_DEBUG("Closing socket for client %llu", id_);
            close(socketFd_);
            
            if (readThread_.joinable()) {
                LOG_DEBUG("Joining read thread for client %llu", id_);
                readThread_.join();
                LOG_DEBUG("Read thread joined for client %llu", id_);
            }
            
            // 只在之前是连接状态的情况下调用回调，避免重复调用
            if (disconnectCallback_) {
                // 将回调放在try块中，防止异常导致程序终止
                try {
                    LOG_DEBUG("Calling disconnect callback for client %llu", id_);
                    disconnectCallback_(id_);
                    LOG_DEBUG("Disconnect callback completed for client %llu", id_);
                } catch (const std::exception& e) {
                    // 记录异常但不让它传播
                    LOG_ERROR("Exception in disconnect callback for client %llu: %s", id_, e.what());
                } catch (...) {
                    LOG_ERROR("Unknown exception in disconnect callback for client %llu", id_);
                }
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in disconnect for client %llu: %s", id_, e.what());
        } catch (...) {
            LOG_ERROR("Unknown exception in disconnect for client %llu", id_);
        }
    }
}

bool TcpConnection::send(const std::string& message) {
    if (!connected_) {
        LOG_DEBUG("Attempt to send to disconnected client %llu", id_);
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
            LOG_ERROR("Send failed for client %llu: %s", id_, strerror(errno));
            connected_ = false;
            return false;
        }
        
        totalSent += sent;
    }
    
    return true;
}

void TcpConnection::startReading() {
    LOG_DEBUG("Starting read thread for client %llu", id_);
    readThread_ = std::thread(&TcpConnection::readLoop, this);
}

void TcpConnection::readLoop() {
    LOG_DEBUG("Read loop started for client %llu", id_);
    const size_t bufferSize = 4096;
    char buffer[bufferSize];
    
    try {
        while (connected_) {
            std::memset(buffer, 0, bufferSize);
            
            ssize_t bytesRead = 0;
            try {
                LOG_DEBUG("Waiting for data from client %llu", id_);
                bytesRead = recv(socketFd_, buffer, bufferSize - 1, 0);
                LOG_DEBUG("Received %zd bytes from client %llu", bytesRead, id_);
            } catch (...) {
                // 捕获潜在的接收异常
                LOG_ERROR("Exception in recv for client %llu", id_);
                break;
            }
            
            if (bytesRead > 0) {
                if (messageCallback_) {
                    try {
                        LOG_DEBUG("Calling message callback for client %llu", id_);
                        messageCallback_(id_, std::string(buffer, bytesRead));
                        LOG_DEBUG("Message callback completed for client %llu", id_);
                    } catch (const std::exception& e) {
                        LOG_ERROR("Exception in message callback for client %llu: %s", id_, e.what());
                    } catch (...) {
                        LOG_ERROR("Unknown exception in message callback for client %llu", id_);
                    }
                }
            } else if (bytesRead == 0) {
                // 客户端关闭连接
                LOG_DEBUG("Client %llu closed connection (bytesRead = 0)", id_);
                break;
            } else {
                // 错误
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    LOG_ERROR("Recv error for client %llu: %s", id_, strerror(errno));
                    break;
                }
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in read loop for client %llu: %s", id_, e.what());
    } catch (...) {
        LOG_ERROR("Unknown exception in read loop for client %llu", id_);
    }
    
    LOG_DEBUG("Read loop ended for client %llu, connected status: %d", id_, connected_.load());
    
    // 确保我们只调用一次disconnect回调
    bool expected = true;
    if (connected_.compare_exchange_strong(expected, false)) {
        LOG_DEBUG("Connected status changed to false in readLoop for client %llu", id_);
        try {
            LOG_DEBUG("Closing socket in readLoop for client %llu", id_);
            close(socketFd_);
            if (disconnectCallback_) {
                try {
                    LOG_DEBUG("Calling disconnect callback from readLoop for client %llu", id_);
                    disconnectCallback_(id_);
                    LOG_DEBUG("Disconnect callback from readLoop completed for client %llu", id_);
                } catch (const std::exception& e) {
                    LOG_ERROR("Exception in disconnect callback from readLoop for client %llu: %s", id_, e.what());
                } catch (...) {
                    LOG_ERROR("Unknown exception in disconnect callback from readLoop for client %llu", id_);
                }
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Exception closing socket in readLoop for client %llu: %s", id_, e.what());
        } catch (...) {
            LOG_ERROR("Unknown exception closing socket in readLoop for client %llu", id_);
        }
    } else {
        LOG_DEBUG("Connected status was already false in readLoop for client %llu", id_);
    }
}
} 