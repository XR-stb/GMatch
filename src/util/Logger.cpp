#include "Logger.h"

namespace gmatch {

Logger::Logger() {
}

Logger::~Logger() {
    if (fileStream_.is_open()) {
        fileStream_.close();
    }
}

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::setLogLevel(LogLevel level) {
    currentLevel_ = level;
}

void Logger::setLogFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (fileStream_.is_open()) {
        fileStream_.close();
    }
    
    fileStream_.open(filename, std::ios::out | std::ios::app);
    if (!fileStream_.is_open()) {
        std::cerr << "Failed to open log file: " << filename << std::endl;
    }
}

std::string Logger::formatLog(LogLevel level, const std::string& message) {
    std::ostringstream oss;
    oss << "[" << getTimestamp() << "] [" << getLevelString(level) << "] " << message;
    return oss.str();
}

std::string Logger::getLevelString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR:   return "ERROR";
        case LogLevel::FATAL:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

std::string Logger::getTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "." 
        << std::setfill('0') << std::setw(3) << ms.count();
    
    return oss.str();
}

} // namespace gmatch 