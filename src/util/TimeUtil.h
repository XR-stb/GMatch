#pragma once

#include <cstdint>
#include <chrono>
#include <string>

namespace gmatch {

class TimeUtil {
public:
    // 获取当前时间戳（毫秒）
    static uint64_t currentTimeMillis();
    
    // 获取当前时间戳（秒）
    static uint64_t currentTimeSeconds();
    
    // 获取格式化的当前时间字符串
    static std::string getCurrentTimeString(const std::string& format = "%Y-%m-%d %H:%M:%S");
    
    // 计算两个时间戳之间的差（毫秒）
    static int64_t timeDiffMillis(uint64_t start, uint64_t end);
    
    // 将毫秒时间戳格式化为可读字符串
    static std::string formatTimeMillis(uint64_t millis, const std::string& format = "%Y-%m-%d %H:%M:%S");
    
    // 睡眠指定毫秒数
    static void sleepMillis(uint32_t millis);
};

} // namespace gmatch 