#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <memory>

namespace gmatch {

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class Logger {
public:
    static Logger& getInstance();
    
    void setLogLevel(LogLevel level);
    void setLogFile(const std::string& filename);
    
    template<typename... Args>
    void debug(const std::string& format, Args&&... args) {
        log(LogLevel::DEBUG, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void info(const std::string& format, Args&&... args) {
        log(LogLevel::INFO, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void warning(const std::string& format, Args&&... args) {
        log(LogLevel::WARNING, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void error(const std::string& format, Args&&... args) {
        log(LogLevel::ERROR, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void fatal(const std::string& format, Args&&... args) {
        log(LogLevel::FATAL, format, std::forward<Args>(args)...);
    }
    
private:
    Logger();
    ~Logger();
    
    template<typename... Args>
    void log(LogLevel level, const std::string& format, Args&&... args) {
        if (level < currentLevel_) {
            return;
        }
        
        std::string message = formatString(format, std::forward<Args>(args)...);
        std::string formattedLog = formatLog(level, message);
        
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << formattedLog << std::endl;
        
        if (fileStream_.is_open()) {
            fileStream_ << formattedLog << std::endl;
        }
    }
    
    template<typename... Args>
    std::string formatString(const std::string& format, Args&&... args) {
        size_t size = std::snprintf(nullptr, 0, format.c_str(), std::forward<Args>(args)...) + 1;
        std::unique_ptr<char[]> buf(new char[size]);
        std::snprintf(buf.get(), size, format.c_str(), std::forward<Args>(args)...);
        return std::string(buf.get(), buf.get() + size - 1);
    }
    
    std::string formatLog(LogLevel level, const std::string& message);
    std::string getLevelString(LogLevel level) const;
    std::string getTimestamp() const;
    
    LogLevel currentLevel_ = LogLevel::INFO;
    std::ofstream fileStream_;
    std::mutex mutex_;
};

#define LOG_DEBUG(...) gmatch::Logger::getInstance().debug(__VA_ARGS__)
#define LOG_INFO(...) gmatch::Logger::getInstance().info(__VA_ARGS__)
#define LOG_WARNING(...) gmatch::Logger::getInstance().warning(__VA_ARGS__)
#define LOG_ERROR(...) gmatch::Logger::getInstance().error(__VA_ARGS__)
#define LOG_FATAL(...) gmatch::Logger::getInstance().fatal(__VA_ARGS__)

} // namespace gmatch 