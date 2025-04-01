#include "MatchServer.h"
#include "../util/Logger.h"
#include "../util/Config.h"
#include <sstream>

namespace gmatch {

MatchServer::MatchServer(const std::string& address, uint16_t port) {
    server_ = std::make_unique<TcpServer>(address, port);
    requestHandler_ = std::make_unique<JsonRequestHandler>();
    
    // 设置回调函数
    server_->setConnectionCallback([this](const TcpConnectionPtr& conn) {
        onClientConnected(conn);
    });
    
    server_->setMessageCallback([this](const TcpConnectionPtr& conn, const std::string& message) {
        onClientMessage(conn, message);
    });
    
    server_->setCloseCallback([this](const TcpConnectionPtr& conn) {
        onClientDisconnected(conn);
    });
    
    // 初始化匹配管理器
    auto& matchManager = MatchManager::getInstance();
    matchManager.init();
    
    // 设置匹配通知回调
    matchManager.setMatchNotifyCallback([this](const RoomPtr& room) {
        onMatchNotify(room);
    });
    
    // 设置玩家状态变更回调
    matchManager.setPlayerStatusCallback([this](Player::PlayerId playerId, bool inQueue) {
        onPlayerStatusChanged(playerId, inQueue);
    });
    
    initialized_ = true;
}

MatchServer::~MatchServer() {
    stop();
}

bool MatchServer::start() {
    if (!initialized_) {
        return false;
    }
    
    LOG_INFO("Starting match server...");
    return server_->start();
}

void MatchServer::stop() {
    if (server_ && server_->isRunning()) {
        LOG_INFO("Stopping match server...");
        server_->stop();
    }
    
    // 关闭匹配管理器
    MatchManager::getInstance().shutdown();
}

void MatchServer::setPlayersPerRoom(int playersPerRoom) {
    auto& config = Config::getInstance();
    config.set("players_per_room", playersPerRoom);
    
    // 重新初始化匹配管理器
    auto& matchManager = MatchManager::getInstance();
    matchManager.shutdown();
    matchManager.init(playersPerRoom);
}

void MatchServer::setMaxRatingDifference(int maxDiff) {
    auto& config = Config::getInstance();
    config.set("max_rating_diff", maxDiff);
    
    // 设置评分差异限制
    MatchManager::getInstance().setMaxRatingDifference(maxDiff);
}

void MatchServer::setLogLevel(LogLevel level) {
    Logger::getInstance().setLogLevel(level);
}

void MatchServer::setLogFile(const std::string& filename) {
    Logger::getInstance().setLogFile(filename);
}

bool MatchServer::isRunning() const {
    return server_ && server_->isRunning();
}

void MatchServer::onClientConnected(const TcpConnectionPtr& conn) {
    LOG_INFO("Client connected: %llu", conn->getId());
}

void MatchServer::onClientMessage(const TcpConnectionPtr& conn, const std::string& message) {
    LOG_DEBUG("Received message from client %llu: %s", conn->getId(), message.c_str());
    
    // 处理请求
    std::string response = requestHandler_->handleRequest(message, conn->getId());
    
    // 发送响应
    conn->send(response);
}

void MatchServer::onClientDisconnected(const TcpConnectionPtr& conn) {
    LOG_INFO("Client disconnected: %llu", conn->getId());
    
    // 如果客户端有关联的玩家，清理相关资源
    std::lock_guard<std::mutex> lock(clientMapMutex_);
    auto it = clientPlayerMap_.find(conn->getId());
    if (it != clientPlayerMap_.end()) {
        MatchManager::getInstance().removePlayer(it->second);
        clientPlayerMap_.erase(it);
    }
}

void MatchServer::onMatchNotify(const RoomPtr& room) {
    if (!room) {
        return;
    }
    
    LOG_INFO("Match found! Room ID: %llu, Players: %d/%d", 
             room->getId(), room->getPlayerCount(), room->getCapacity());
    
    // 创建通知消息
    std::ostringstream oss;
    oss << "{\"room_id\":" << room->getId()
        << ",\"players\":[";
    
    auto players = room->getPlayers();
    for (size_t i = 0; i < players.size(); ++i) {
        if (i > 0) {
            oss << ",";
        }
        oss << "{\"player_id\":" << players[i]->getId()
            << ",\"name\":\"" << players[i]->getName()
            << "\",\"rating\":" << players[i]->getRating()
            << "}";
    }
    
    oss << "]}";
    
    std::string notification = "{\"cmd\":\"match_notify\",\"success\":true,\"message\":\"Match found\",\"data\":"
                               + oss.str() + "}";
    
    // 向所有匹配的玩家发送通知
    std::lock_guard<std::mutex> lock(clientMapMutex_);
    for (const auto& player : players) {
        for (const auto& pair : clientPlayerMap_) {
            if (pair.second == player->getId()) {
                server_->sendToClient(pair.first, notification);
                break;
            }
        }
    }
}

void MatchServer::onPlayerStatusChanged(Player::PlayerId playerId, bool inQueue) {
    LOG_DEBUG("Player %llu %s queue", playerId, inQueue ? "joined" : "left");
    
    // 向客户端发送状态变更通知
    std::string status = inQueue ? "in_queue" : "left_queue";
    std::string notification = "{\"cmd\":\"status_changed\",\"success\":true,\"message\":\"Player status changed\","
                               "\"data\":{\"player_id\":" + std::to_string(playerId) + 
                               ",\"status\":\"" + status + "\"}}";
    
    std::lock_guard<std::mutex> lock(clientMapMutex_);
    for (const auto& pair : clientPlayerMap_) {
        if (pair.second == playerId) {
            server_->sendToClient(pair.first, notification);
            break;
        }
    }
}

} // namespace gmatch 