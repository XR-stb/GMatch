#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <any>
#include <fstream>
#include <iostream>
#include <sstream>

namespace gmatch {

class Config {
public:
    static Config& getInstance();
    
    bool loadFromFile(const std::string& filename);
    bool saveToFile(const std::string& filename) const;
    
    // 设置配置项
    template<typename T>
    void set(const std::string& key, const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_[key] = value;
    }
    
    // 获取配置项
    template<typename T>
    T get(const std::string& key, const T& defaultValue = T()) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = config_.find(key);
        if (it != config_.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast&) {
                std::cerr << "Type mismatch for config key: " << key << std::endl;
                return defaultValue;
            }
        }
        return defaultValue;
    }
    
    // 检查配置项是否存在
    bool hasKey(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_.find(key) != config_.end();
    }
    
    // 清空所有配置
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        config_.clear();
    }
    
private:
    Config() = default;
    ~Config() = default;
    
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::any> config_;
};

} // namespace gmatch 