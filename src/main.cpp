#include <iostream>
#include <string>
#include <csignal>
#include <thread>
#include "server/MatchServer.h"
#include "util/Logger.h"
#include "util/Config.h"

using namespace gmatch;

// 全局服务器实例
std::unique_ptr<MatchServer> g_server;

// 信号处理
void signalHandler(int signal) {
    LOG_INFO("Received signal %d, shutting down...", signal);
    if (g_server) {
        g_server->stop();
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    // 设置信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // 默认配置
    std::string address = "0.0.0.0";
    uint16_t port = 8080;
    int playersPerRoom = 2;
    int maxRatingDiff = 300;
    std::string logFile = "match_server.log";
    LogLevel logLevel = LogLevel::INFO;
    
    // 加载配置文件
    auto& config = Config::getInstance();
    if (config.loadFromFile("config.ini")) {
        address = config.get<std::string>("address", address);
        port = config.get<int>("port", port);
        playersPerRoom = config.get<int>("players_per_room", playersPerRoom);
        maxRatingDiff = config.get<int>("max_rating_diff", maxRatingDiff);
        logFile = config.get<std::string>("log_file", logFile);
        
        int logLevelInt = config.get<int>("log_level", static_cast<int>(logLevel));
        logLevel = static_cast<LogLevel>(logLevelInt);
    } else {
        // 创建默认配置
        config.set("address", address);
        config.set("port", port);
        config.set("players_per_room", playersPerRoom);
        config.set("max_rating_diff", maxRatingDiff);
        config.set("log_file", logFile);
        config.set("log_level", static_cast<int>(logLevel));
        
        config.saveToFile("config.ini");
    }
    
    // 设置日志
    auto& logger = Logger::getInstance();
    logger.setLogLevel(logLevel);
    logger.setLogFile(logFile);
    
    LOG_INFO("Starting GMatch server...");
    LOG_INFO("Address: %s", address.c_str());
    LOG_INFO("Port: %d", port);
    LOG_INFO("Players per room: %d", playersPerRoom);
    LOG_INFO("Max rating difference: %d", maxRatingDiff);
    
    // 创建并启动服务器
    g_server = std::make_unique<MatchServer>(address, port);
    g_server->setPlayersPerRoom(playersPerRoom);
    g_server->setMaxRatingDifference(maxRatingDiff);
    
    if (!g_server->start()) {
        LOG_FATAL("Failed to start server");
        return 1;
    }
    
    LOG_INFO("Server is running. Press Ctrl+C to stop.");
    
    // 主线程等待，让工作线程继续运行
    while (g_server->isRunning()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
} 