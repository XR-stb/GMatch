#include "RequestHandler.h"
#include <sstream>
#include <iostream>
#include "../util/Logger.h"

// 简单的JSON解析与生成，在实际环境中可以使用第三方库如nlohmann/json或RapidJSON
namespace gmatch {

JsonRequestHandler::JsonRequestHandler() {
    // 注册默认命令处理器
    registerCommandHandler("create_player", [this](const std::string& data, TcpConnection::ConnectionId clientId) {
        return handleCreatePlayer(data, clientId);
    });
    
    registerCommandHandler("join_matchmaking", [this](const std::string& data, TcpConnection::ConnectionId clientId) {
        return handleJoinMatchmaking(data, clientId);
    });
    
    registerCommandHandler("leave_matchmaking", [this](const std::string& data, TcpConnection::ConnectionId clientId) {
        return handleLeaveMatchmaking(data, clientId);
    });
    
    registerCommandHandler("get_rooms", [this](const std::string& data, TcpConnection::ConnectionId clientId) {
        return handleGetRooms(data, clientId);
    });
    
    registerCommandHandler("get_player_info", [this](const std::string& data, TcpConnection::ConnectionId clientId) {
        return handleGetPlayerInfo(data, clientId);
    });
    
    registerCommandHandler("get_queue_status", [this](const std::string& data, TcpConnection::ConnectionId clientId) {
        return handleGetQueueStatus(data, clientId);
    });
}

std::string JsonRequestHandler::handleRequest(const std::string& request, TcpConnection::ConnectionId clientId) {
    std::string command, data;
    
    if (!parseJsonRequest(request, command, data)) {
        return createJsonResponse("error", false, "Invalid JSON format", "");
    }
    
    LOG_DEBUG("Received command: %s, data: %s", command.c_str(), data.c_str());
    
    auto it = commandHandlers_.find(command);
    if (it != commandHandlers_.end()) {
        return it->second(data, clientId);
    } else {
        return createJsonResponse(command, false, "Unknown command", "");
    }
}

void JsonRequestHandler::registerCommandHandler(const std::string& command, CommandHandler handler) {
    commandHandlers_[command] = handler;
}

bool JsonRequestHandler::parseJsonRequest(const std::string& request, std::string& command, std::string& data) {
    // 简单的JSON解析
    // 格式假设为: {"cmd":"命令名","data":{...}}
    size_t cmdStart = request.find("\"cmd\"");
    size_t dataStart = request.find("\"data\"");
    
    if (cmdStart == std::string::npos || dataStart == std::string::npos) {
        return false;
    }
    
    // 解析命令
    cmdStart = request.find(':', cmdStart) + 1;
    size_t cmdEnd = request.find(',', cmdStart);
    if (cmdEnd == std::string::npos) {
        cmdEnd = request.find('}', cmdStart);
    }
    
    std::string cmdValue = request.substr(cmdStart, cmdEnd - cmdStart);
    // 去除引号和空白
    cmdValue.erase(std::remove_if(cmdValue.begin(), cmdValue.end(), 
                                   [](char c) { return c == '\"' || c == ' '; }), 
                   cmdValue.end());
    command = cmdValue;
    
    // 解析数据
    dataStart = request.find(':', dataStart) + 1;
    size_t dataEnd = request.rfind('}');
    if (dataStart >= dataEnd) {
        data = "{}";
    } else {
        data = request.substr(dataStart, dataEnd - dataStart + 1);
    }
    
    return true;
}

std::string JsonRequestHandler::createJsonResponse(const std::string& command, bool success, 
                                                  const std::string& message, const std::string& data) {
    std::ostringstream oss;
    oss << "{\"cmd\":\"" << command << "\",\"success\":" << (success ? "true" : "false") 
        << ",\"message\":\"" << message << "\"";
        
    if (!data.empty()) {
        oss << ",\"data\":" << data;
    }
    
    oss << "}";
    return oss.str();
}

std::string JsonRequestHandler::handleCreatePlayer(const std::string& data, TcpConnection::ConnectionId clientId) {
    // 解析玩家名称和评分
    std::string name = "Player";
    int rating = 1500;
    
    // 简单的数据解析
    size_t namePos = data.find("\"name\"");
    size_t ratingPos = data.find("\"rating\"");
    
    if (namePos != std::string::npos) {
        namePos = data.find(':', namePos) + 1;
        size_t nameEnd = data.find(',', namePos);
        if (nameEnd == std::string::npos) {
            nameEnd = data.find('}', namePos);
        }
        
        std::string nameValue = data.substr(namePos, nameEnd - namePos);
        // 去除引号和空白
        nameValue.erase(std::remove_if(nameValue.begin(), nameValue.end(), 
                                     [](char c) { return c == '\"' || c == ' '; }), 
                       nameValue.end());
        if (!nameValue.empty()) {
            name = nameValue;
        }
    }
    
    if (ratingPos != std::string::npos) {
        ratingPos = data.find(':', ratingPos) + 1;
        size_t ratingEnd = data.find(',', ratingPos);
        if (ratingEnd == std::string::npos) {
            ratingEnd = data.find('}', ratingPos);
        }
        
        std::string ratingValue = data.substr(ratingPos, ratingEnd - ratingPos);
        // 去除空白
        ratingValue.erase(std::remove_if(ratingValue.begin(), ratingValue.end(), 
                                       [](char c) { return c == ' '; }), 
                         ratingValue.end());
        try {
            rating = std::stoi(ratingValue);
        } catch (...) {
            // 使用默认评分
        }
    }
    
    // 创建玩家
    auto& matchManager = MatchManager::getInstance();
    auto player = matchManager.createPlayer(name, rating);
    
    if (player) {
        std::ostringstream oss;
        oss << "{\"player_id\":" << player->getId() 
            << ",\"name\":\"" << player->getName() 
            << "\",\"rating\":" << player->getRating() << "}";
        
        return createJsonResponse("create_player", true, "Player created successfully", oss.str());
    } else {
        return createJsonResponse("create_player", false, "Failed to create player", "");
    }
}

std::string JsonRequestHandler::handleJoinMatchmaking(const std::string& data, TcpConnection::ConnectionId clientId) {
    // 解析玩家ID
    Player::PlayerId playerId = 0;
    
    size_t playerIdPos = data.find("\"player_id\"");
    if (playerIdPos != std::string::npos) {
        playerIdPos = data.find(':', playerIdPos) + 1;
        size_t playerIdEnd = data.find(',', playerIdPos);
        if (playerIdEnd == std::string::npos) {
            playerIdEnd = data.find('}', playerIdPos);
        }
        
        std::string playerIdValue = data.substr(playerIdPos, playerIdEnd - playerIdPos);
        // 去除空白
        playerIdValue.erase(std::remove_if(playerIdValue.begin(), playerIdValue.end(), 
                                         [](char c) { return c == ' '; }), 
                           playerIdValue.end());
        try {
            playerId = std::stoull(playerIdValue);
        } catch (...) {
            return createJsonResponse("join_matchmaking", false, "Invalid player ID", "");
        }
    } else {
        return createJsonResponse("join_matchmaking", false, "Player ID is required", "");
    }
    
    // 加入匹配队列
    auto& matchManager = MatchManager::getInstance();
    bool success = matchManager.joinMatchmaking(playerId);
    
    if (success) {
        return createJsonResponse("join_matchmaking", true, "Joined matchmaking queue", "");
    } else {
        return createJsonResponse("join_matchmaking", false, "Failed to join matchmaking queue", "");
    }
}

std::string JsonRequestHandler::handleLeaveMatchmaking(const std::string& data, TcpConnection::ConnectionId clientId) {
    // 解析玩家ID
    Player::PlayerId playerId = 0;
    
    size_t playerIdPos = data.find("\"player_id\"");
    if (playerIdPos != std::string::npos) {
        playerIdPos = data.find(':', playerIdPos) + 1;
        size_t playerIdEnd = data.find(',', playerIdPos);
        if (playerIdEnd == std::string::npos) {
            playerIdEnd = data.find('}', playerIdPos);
        }
        
        std::string playerIdValue = data.substr(playerIdPos, playerIdEnd - playerIdPos);
        // 去除空白
        playerIdValue.erase(std::remove_if(playerIdValue.begin(), playerIdValue.end(), 
                                         [](char c) { return c == ' '; }), 
                           playerIdValue.end());
        try {
            playerId = std::stoull(playerIdValue);
        } catch (...) {
            return createJsonResponse("leave_matchmaking", false, "Invalid player ID", "");
        }
    } else {
        return createJsonResponse("leave_matchmaking", false, "Player ID is required", "");
    }
    
    // 离开匹配队列
    auto& matchManager = MatchManager::getInstance();
    bool success = matchManager.leaveMatchmaking(playerId);
    
    if (success) {
        return createJsonResponse("leave_matchmaking", true, "Left matchmaking queue", "");
    } else {
        return createJsonResponse("leave_matchmaking", false, "Failed to leave matchmaking queue", "");
    }
}

std::string JsonRequestHandler::handleGetRooms(const std::string& data, TcpConnection::ConnectionId clientId) {
    auto& matchManager = MatchManager::getInstance();
    auto rooms = matchManager.getAllRooms();
    
    std::ostringstream oss;
    oss << "[";
    
    bool first = true;
    for (const auto& room : rooms) {
        if (!first) {
            oss << ",";
        }
        first = false;
        
        oss << "{\"room_id\":" << room->getId()
            << ",\"status\":" << static_cast<int>(room->getStatus())
            << ",\"player_count\":" << room->getPlayerCount()
            << ",\"capacity\":" << room->getCapacity()
            << ",\"avg_rating\":" << room->getAverageRating()
            << "}";
    }
    
    oss << "]";
    
    return createJsonResponse("get_rooms", true, "Rooms retrieved successfully", oss.str());
}

std::string JsonRequestHandler::handleGetPlayerInfo(const std::string& data, TcpConnection::ConnectionId clientId) {
    // 解析玩家ID
    Player::PlayerId playerId = 0;
    
    size_t playerIdPos = data.find("\"player_id\"");
    if (playerIdPos != std::string::npos) {
        playerIdPos = data.find(':', playerIdPos) + 1;
        size_t playerIdEnd = data.find(',', playerIdPos);
        if (playerIdEnd == std::string::npos) {
            playerIdEnd = data.find('}', playerIdPos);
        }
        
        std::string playerIdValue = data.substr(playerIdPos, playerIdEnd - playerIdPos);
        // 去除空白
        playerIdValue.erase(std::remove_if(playerIdValue.begin(), playerIdValue.end(), 
                                         [](char c) { return c == ' '; }), 
                           playerIdValue.end());
        try {
            playerId = std::stoull(playerIdValue);
        } catch (...) {
            return createJsonResponse("get_player_info", false, "Invalid player ID", "");
        }
    } else {
        return createJsonResponse("get_player_info", false, "Player ID is required", "");
    }
    
    // 获取玩家信息
    auto& matchManager = MatchManager::getInstance();
    auto player = matchManager.getPlayer(playerId);
    
    if (player) {
        std::ostringstream oss;
        oss << "{\"player_id\":" << player->getId() 
            << ",\"name\":\"" << player->getName() 
            << "\",\"rating\":" << player->getRating()
            << ",\"in_queue\":" << (player->isInQueue() ? "true" : "false")
            << "}";
        
        return createJsonResponse("get_player_info", true, "Player info retrieved successfully", oss.str());
    } else {
        return createJsonResponse("get_player_info", false, "Player not found", "");
    }
}

std::string JsonRequestHandler::handleGetQueueStatus(const std::string& data, TcpConnection::ConnectionId clientId) {
    auto& matchManager = MatchManager::getInstance();
    size_t queueSize = matchManager.getQueueSize();
    
    std::ostringstream oss;
    oss << "{\"queue_size\":" << queueSize << "}";
    
    return createJsonResponse("get_queue_status", true, "Queue status retrieved successfully", oss.str());
}

} // namespace gmatch 