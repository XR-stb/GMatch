#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <sstream>
#include "MatchClient.h"

using namespace gmatch;

std::atomic<bool> g_running(true);

void handleEvent(const ClientEvent& event) {
    std::cout << "Event: ";
    
    switch (event.type) {
        case ClientEventType::CONNECTED:
            std::cout << "Connected";
            break;
        case ClientEventType::DISCONNECTED:
            std::cout << "Disconnected";
            g_running = false;
            break;
        case ClientEventType::PLAYER_CREATED:
            std::cout << "Player Created";
            break;
        case ClientEventType::JOINED_QUEUE:
            std::cout << "Joined Queue";
            break;
        case ClientEventType::LEFT_QUEUE:
            std::cout << "Left Queue";
            break;
        case ClientEventType::MATCH_FOUND:
            std::cout << "Match Found";
            break;
        case ClientEventType::ERROR:
            std::cout << "Error";
            break;
        default:
            std::cout << "Unknown";
            break;
    }
    
    std::cout << " - " << event.message << std::endl;
    
    if (!event.data.empty()) {
        std::cout << "Data: " << event.data << std::endl;
    }
}

void showHelp() {
    std::cout << "Commands:" << std::endl;
    std::cout << "  create <name> <rating>  - Create a player" << std::endl;
    std::cout << "  join                    - Join matchmaking" << std::endl;
    std::cout << "  leave                   - Leave matchmaking" << std::endl;
    std::cout << "  rooms                   - Get room list" << std::endl;
    std::cout << "  info                    - Get player info" << std::endl;
    std::cout << "  queue                   - Get queue status" << std::endl;
    std::cout << "  exit                    - Exit" << std::endl;
    std::cout << "  help                    - Show this help" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string address = "127.0.0.1";
    uint16_t port = 8080;
    
    // 解析命令行参数
    if (argc >= 2) {
        address = argv[1];
    }
    
    if (argc >= 3) {
        try {
            port = std::stoi(argv[2]);
        } catch (...) {
            std::cerr << "Invalid port number" << std::endl;
            return 1;
        }
    }
    
    std::cout << "Connecting to " << address << ":" << port << std::endl;
    
    MatchClient client;
    client.setEventCallback(handleEvent);
    
    if (!client.connect(address, port)) {
        std::cerr << "Failed to connect to server" << std::endl;
        return 1;
    }
    
    std::cout << "Connected to server" << std::endl;
    showHelp();
    
    std::string line;
    while (g_running && std::getline(std::cin, line)) {
        if (line == "exit") {
            break;
        } else if (line == "help") {
            showHelp();
        } else if (line.substr(0, 6) == "create") {
            std::istringstream iss(line);
            std::string cmd, name;
            int rating = 1500;
            
            iss >> cmd >> name;
            if (iss >> rating) {
                // 读取成功
            }
            
            if (name.empty()) {
                std::cout << "Usage: create <name> <rating>" << std::endl;
                continue;
            }
            
            client.createPlayer(name, rating);
        } else if (line == "join") {
            if (client.getPlayerId() == 0) {
                std::cout << "Create a player first" << std::endl;
                continue;
            }
            
            client.joinMatchmaking();
        } else if (line == "leave") {
            if (client.getPlayerId() == 0) {
                std::cout << "Create a player first" << std::endl;
                continue;
            }
            
            client.leaveMatchmaking();
        } else if (line == "rooms") {
            client.getRooms();
        } else if (line == "info") {
            if (client.getPlayerId() == 0) {
                std::cout << "Create a player first" << std::endl;
                continue;
            }
            
            client.getPlayerInfo();
        } else if (line == "queue") {
            client.getQueueStatus();
        } else {
            std::cout << "Unknown command: " << line << std::endl;
            showHelp();
        }
    }
    
    client.disconnect();
    return 0;
} 