#include <iostream>
#include <string>
#include <csignal>
#include <thread>
#include <cstring>
#include <cstdlib>
#include <execinfo.h>
#include "server/MatchServer.h"
#include "util/Logger.h"
#include "util/Config.h"

using namespace gmatch;

// 全局服务器实例
std::unique_ptr<MatchServer> g_server;

// 检查命令行是否包含特定选项
bool hasOption(char* argv[], int argc, const char* option);

// 打印堆栈跟踪
void print_trace() {
    void *array[20];
    size_t size;
    char **strings;
    size_t i;

    size = backtrace(array, 20);
    strings = backtrace_symbols(array, size);

    std::cerr << "Obtained " << size << " stack frames.\n";

    for (i = 0; i < size; i++) {
        std::cerr << strings[i] << std::endl;
    }

    free(strings);
}

// 信号处理
void signalHandler(int signal) {
    if (signal == SIGSEGV || signal == SIGABRT) {
        std::cerr << "Received signal " << signal << " (";
        if (signal == SIGSEGV) std::cerr << "SIGSEGV";
        else if (signal == SIGABRT) std::cerr << "SIGABRT";
        std::cerr << "), printing stack trace:\n";
        print_trace();
    }

    LOG_INFO("Received signal %d, shutting down...", signal);
    if (g_server) {
        g_server->stop();
    }
    
    // 如果是终止信号，正常退出
    if (signal == SIGINT || signal == SIGTERM) {
        exit(0);
    } else {
        // 如果是崩溃信号，生成core文件
        std::abort();
    }
}

// 显示帮助信息
void showHelp(const char* programName) {
    std::cout << "Usage: " << programName << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --config FILE      Config file path (default: config.ini)" << std::endl;
    std::cout << "  --address ADDR     Server address (default: 0.0.0.0)" << std::endl;
    std::cout << "  --port PORT        Server port (default: 9090)" << std::endl;
    std::cout << "  --players NUM      Players per room (default: 2)" << std::endl;
    std::cout << "  --max-diff NUM     Max rating difference (default: 300)" << std::endl;
    std::cout << "  --log-file FILE    Log file path (default: match_server.log)" << std::endl;
    std::cout << "  --log-level LEVEL  Log level (0=DEBUG, 1=INFO, 2=WARNING, 3=ERROR, 4=FATAL) (default: 1)" << std::endl;
    std::cout << "  --help             Display this help message" << std::endl;
}

int main(int argc, char* argv[]) {
    // 设置信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGSEGV, signalHandler);
    signal(SIGABRT, signalHandler);
    
    // 默认配置
    std::string configFile = "config.ini";
    std::string address = "0.0.0.0";
    uint16_t port = 9090;
    int playersPerRoom = 2;
    int maxRatingDiff = 300;
    std::string logFile = "match_server.log";
    LogLevel logLevel = LogLevel::INFO;
    bool configFileSpecified = false;
    
    // 处理命令行参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            showHelp(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            configFile = argv[++i];
            configFileSpecified = true;
        } else if (strcmp(argv[i], "--address") == 0 && i + 1 < argc) {
            address = argv[++i];
        } else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if (strcmp(argv[i], "--players") == 0 && i + 1 < argc) {
            playersPerRoom = std::stoi(argv[++i]);
        } else if (strcmp(argv[i], "--max-diff") == 0 && i + 1 < argc) {
            maxRatingDiff = std::stoi(argv[++i]);
        } else if (strcmp(argv[i], "--log-file") == 0 && i + 1 < argc) {
            logFile = argv[++i];
        } else if (strcmp(argv[i], "--log-level") == 0 && i + 1 < argc) {
            int level = std::stoi(argv[++i]);
            if (level >= 0 && level <= 4) {
                logLevel = static_cast<LogLevel>(level);
            } else {
                std::cerr << "Invalid log level: " << level << ". Using default." << std::endl;
            }
        } else {
            std::cerr << "Unknown option: " << argv[i] << std::endl;
            showHelp(argv[0]);
            return 1;
        }
    }
    
    // 加载配置文件(如果命令行没有指定参数)
    auto& config = Config::getInstance();
    if (configFileSpecified && config.loadFromFile(configFile)) {
        // 只覆盖命令行未指定的参数
        if (!hasOption(argv, argc, "--address")) {
            address = config.get<std::string>("address", address);
        }
        if (!hasOption(argv, argc, "--port")) {
            port = config.get<int>("port", port);
        }
        if (!hasOption(argv, argc, "--players")) {
            playersPerRoom = config.get<int>("players_per_room", playersPerRoom);
        }
        if (!hasOption(argv, argc, "--max-diff")) {
            maxRatingDiff = config.get<int>("max_rating_diff", maxRatingDiff);
        }
        if (!hasOption(argv, argc, "--log-file")) {
            logFile = config.get<std::string>("log_file", logFile);
        }
        if (!hasOption(argv, argc, "--log-level")) {
            int logLevelInt = config.get<int>("log_level", static_cast<int>(logLevel));
            logLevel = static_cast<LogLevel>(logLevelInt);
        }
    } else if (!configFileSpecified) {
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

// 检查命令行是否包含特定选项
bool hasOption(char* argv[], int argc, const char* option) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], option) == 0) {
            return true;
        }
    }
    return false;
} 