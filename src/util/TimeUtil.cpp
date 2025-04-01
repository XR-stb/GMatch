#include "TimeUtil.h"
#include <iomanip>
#include <sstream>
#include <thread>

namespace gmatch {

uint64_t TimeUtil::currentTimeMillis() {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
}

uint64_t TimeUtil::currentTimeSeconds() {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
}

std::string TimeUtil::getCurrentTimeString(const std::string& format) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), format.c_str());
    return oss.str();
}

int64_t TimeUtil::timeDiffMillis(uint64_t start, uint64_t end) {
    return static_cast<int64_t>(end - start);
}

std::string TimeUtil::formatTimeMillis(uint64_t millis, const std::string& format) {
    auto duration = std::chrono::milliseconds(millis);
    auto timePoint = std::chrono::system_clock::time_point(duration);
    auto time = std::chrono::system_clock::to_time_t(timePoint);
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), format.c_str());
    return oss.str();
}

void TimeUtil::sleepMillis(uint32_t millis) {
    std::this_thread::sleep_for(std::chrono::milliseconds(millis));
}

} // namespace gmatch 