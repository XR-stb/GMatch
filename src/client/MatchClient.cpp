#include "MatchClient.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sstream>

namespace gmatch {

MatchClient::MatchClient() {
}

MatchClient::~MatchClient() {
    disconnect();
}

bool MatchClient::connect(const std::string& address, uint16_t port) {
    if (connected_) {
        return true;
    }
    
    // 创建socket
    socketFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd_ < 0) {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        return false;
    }
    
    // 设置地址
    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, address.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address: " << address << std::endl;
        close(socketFd_);
        socketFd_ = -1;
        return false;
    }
    
    // 连接服务器
    if (::connect(socketFd_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Failed to connect: " << strerror(errno) << std::endl;
        close(socketFd_);
        socketFd_ = -1;
        return false;
    }
    
    connected_ = true;
    running_ = true;
    
    // 启动接收线程
    receiveThread_ = std::thread([this]() {
        const size_t bufferSize = 4096;
        char buffer[bufferSize];
        
        while (running_) {
            std::memset(buffer, 0, bufferSize);
            ssize_t bytesRead = recv(socketFd_, buffer, bufferSize - 1, 0);
            
            if (bytesRead > 0) {
                messageReceived(std::string(buffer, bytesRead));
            } else if (bytesRead == 0) {
                // 服务器关闭连接
                break;
            } else {
                // 错误
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    break;
                }
            }
        }
        
        connected_ = false;
        
        // 通知断开连接
        ClientEvent event;
        event.type = ClientEventType::DISCONNECTED;
        event.message = "Disconnected from server";
        
        std::lock_guard<std::mutex> lock(eventQueueMutex_);
        eventQueue_.push_back(event);
        eventQueueCv_.notify_one();
    });
    
    // 启动事件处理线程
    eventProcessThread_ = std::thread(&MatchClient::processEvents, this);
    
    // 通知连接成功
    ClientEvent event;
    event.type = ClientEventType::CONNECTED;
    event.message = "Connected to server";
    
    std::lock_guard<std::mutex> lock(eventQueueMutex_);
    eventQueue_.push_back(event);
    eventQueueCv_.notify_one();
    
    return true;
}

void MatchClient::disconnect() {
    if (!connected_) {
        return;
    }
    
    running_ = false;
    
    // 关闭socket
    if (socketFd_ >= 0) {
        close(socketFd_);
        socketFd_ = -1;
    }
    
    // 等待线程结束
    if (receiveThread_.joinable()) {
        receiveThread_.join();
    }
    
    if (eventProcessThread_.joinable()) {
        eventProcessThread_.join();
    }
    
    connected_ = false;
}

bool MatchClient::createPlayer(const std::string& name, int rating) {
    std::stringstream ss;
    ss << "{\"name\":\"" << name << "\",\"rating\":" << rating << "}";
    return sendRequest("create_player", ss.str());
}

bool MatchClient::joinMatchmaking() {
    if (playerId_ == 0) {
        return false;
    }
    
    std::stringstream ss;
    ss << "{\"player_id\":" << playerId_ << "}";
    return sendRequest("join_matchmaking", ss.str());
}

bool MatchClient::leaveMatchmaking() {
    if (playerId_ == 0) {
        return false;
    }
    
    std::stringstream ss;
    ss << "{\"player_id\":" << playerId_ << "}";
    return sendRequest("leave_matchmaking", ss.str());
}

bool MatchClient::getRooms() {
    return sendRequest("get_rooms", "{}");
}

bool MatchClient::getPlayerInfo() {
    if (playerId_ == 0) {
        return false;
    }
    
    std::stringstream ss;
    ss << "{\"player_id\":" << playerId_ << "}";
    return sendRequest("get_player_info", ss.str());
}

bool MatchClient::getQueueStatus() {
    return sendRequest("get_queue_status", "{}");
}

void MatchClient::setEventCallback(EventCallback callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    eventCallback_ = callback;
}

bool MatchClient::isConnected() const {
    return connected_;
}

Player::PlayerId MatchClient::getPlayerId() const {
    return playerId_;
}

Room::RoomId MatchClient::getRoomId() const {
    return roomId_;
}

void MatchClient::processEvents() {
    while (running_) {
        ClientEvent event;
        
        {
            std::unique_lock<std::mutex> lock(eventQueueMutex_);
            eventQueueCv_.wait(lock, [this] { return !eventQueue_.empty() || !running_; });
            
            if (!running_ && eventQueue_.empty()) {
                break;
            }
            
            event = eventQueue_.front();
            eventQueue_.pop_front();
        }
        
        // 调用回调
        std::lock_guard<std::mutex> lock(callbackMutex_);
        if (eventCallback_) {
            eventCallback_(event);
        }
    }
}

void MatchClient::messageReceived(const std::string& message) {
    processResponse(message);
}

void MatchClient::processResponse(const std::string& response) {
    // 解析响应
    // 格式: {"cmd":"命令","success":true/false,"message":"消息","data":{...}}
    
    size_t cmdStart = response.find("\"cmd\"");
    size_t successStart = response.find("\"success\"");
    size_t messageStart = response.find("\"message\"");
    size_t dataStart = response.find("\"data\"");
    
    if (cmdStart == std::string::npos || successStart == std::string::npos || messageStart == std::string::npos) {
        return;
    }
    
    // 解析命令
    cmdStart = response.find(':', cmdStart) + 1;
    size_t cmdEnd = response.find(',', cmdStart);
    std::string cmd = response.substr(cmdStart, cmdEnd - cmdStart);
    cmd.erase(std::remove_if(cmd.begin(), cmd.end(), [](char c) { return c == '\"' || c == ' '; }), cmd.end());
    
    // 解析成功状态
    successStart = response.find(':', successStart) + 1;
    size_t successEnd = response.find(',', successStart);
    std::string successStr = response.substr(successStart, successEnd - successStart);
    successStr.erase(std::remove_if(successStr.begin(), successStr.end(), 
                                   [](char c) { return c == ' '; }), 
                    successStr.end());
    bool success = (successStr == "true");
    
    // 解析消息
    messageStart = response.find(':', messageStart) + 1;
    size_t messageEnd = response.find(',', messageStart);
    if (messageEnd == std::string::npos || dataStart == std::string::npos) {
        messageEnd = response.find('}', messageStart);
    }
    std::string message = response.substr(messageStart, messageEnd - messageStart);
    message.erase(std::remove_if(message.begin(), message.end(), 
                                [](char c) { return c == '\"'; }), 
                 message.end());
    
    // 解析数据
    std::string data;
    if (dataStart != std::string::npos) {
        dataStart = response.find(':', dataStart) + 1;
        size_t dataEnd = response.rfind('}');
        data = response.substr(dataStart, dataEnd - dataStart + 1);
    }
    
    ClientEvent event;
    event.message = message;
    event.data = data;
    
    if (!success) {
        event.type = ClientEventType::ERROR;
    } else if (cmd == "create_player") {
        event.type = ClientEventType::PLAYER_CREATED;
        
        // 提取玩家ID
        if (!data.empty()) {
            size_t playerIdPos = data.find("\"player_id\"");
            if (playerIdPos != std::string::npos) {
                playerIdPos = data.find(':', playerIdPos) + 1;
                size_t playerIdEnd = data.find(',', playerIdPos);
                if (playerIdEnd == std::string::npos) {
                    playerIdEnd = data.find('}', playerIdPos);
                }
                
                std::string playerIdStr = data.substr(playerIdPos, playerIdEnd - playerIdPos);
                try {
                    playerId_ = std::stoull(playerIdStr);
                } catch (...) {
                    // 忽略解析错误
                }
            }
        }
    } else if (cmd == "join_matchmaking") {
        event.type = ClientEventType::JOINED_QUEUE;
    } else if (cmd == "leave_matchmaking") {
        event.type = ClientEventType::LEFT_QUEUE;
    } else if (cmd == "match_notify") {
        event.type = ClientEventType::MATCH_FOUND;
        
        // 提取房间ID
        if (!data.empty()) {
            size_t roomIdPos = data.find("\"room_id\"");
            if (roomIdPos != std::string::npos) {
                roomIdPos = data.find(':', roomIdPos) + 1;
                size_t roomIdEnd = data.find(',', roomIdPos);
                if (roomIdEnd == std::string::npos) {
                    roomIdEnd = data.find('}', roomIdPos);
                }
                
                std::string roomIdStr = data.substr(roomIdPos, roomIdEnd - roomIdPos);
                try {
                    roomId_ = std::stoull(roomIdStr);
                } catch (...) {
                    // 忽略解析错误
                }
            }
        }
    } else {
        // 其他响应，不产生特定事件
        return;
    }
    
    // 添加到事件队列
    std::lock_guard<std::mutex> lock(eventQueueMutex_);
    eventQueue_.push_back(event);
    eventQueueCv_.notify_one();
}

bool MatchClient::sendRequest(const std::string& cmd, const std::string& data) {
    if (!connected_) {
        return false;
    }
    
    std::stringstream ss;
    ss << "{\"cmd\":\"" << cmd << "\",\"data\":" << data << "}";
    std::string request = ss.str();
    
    size_t totalSent = 0;
    size_t requestSize = request.size();
    const char* buffer = request.c_str();
    
    while (totalSent < requestSize) {
        ssize_t sent = ::send(socketFd_, buffer + totalSent, requestSize - totalSent, 0);
        if (sent <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 缓冲区已满，稍后重试
                continue;
            }
            return false;
        }
        
        totalSent += sent;
    }
    
    return true;
}

} // namespace gmatch 