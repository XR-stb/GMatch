#include "Config.h"
#include <fstream>
#include <sstream>

namespace gmatch {

Config& Config::getInstance() {
    static Config instance;
    return instance;
}

bool Config::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    clear();
    std::string line;
    while (std::getline(file, line)) {
        // 跳过注释和空行
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        std::istringstream iss(line);
        std::string key, value;
        
        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            // 去除首尾空白
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // 尝试转换为数值类型
            if (value.find('.') != std::string::npos) {
                try {
                    double doubleVal = std::stod(value);
                    set(key, doubleVal);
                } catch (...) {
                    set(key, value);
                }
            } else {
                try {
                    int intVal = std::stoi(value);
                    set(key, intVal);
                } catch (...) {
                    set(key, value);
                }
            }
        }
    }
    
    file.close();
    return true;
}

bool Config::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    file << "# GMatch Configuration File\n";
    file << "# Generated on " << std::time(nullptr) << "\n\n";
    
    for (const auto& pair : config_) {
        file << pair.first << " = ";
        
        // 尝试以不同类型输出
        try {
            file << std::any_cast<int>(pair.second);
        } catch (const std::bad_any_cast&) {
            try {
                file << std::any_cast<double>(pair.second);
            } catch (const std::bad_any_cast&) {
                try {
                    file << std::any_cast<std::string>(pair.second);
                } catch (const std::bad_any_cast&) {
                    file << "[complex type]";
                }
            }
        }
        
        file << std::endl;
    }
    
    file.close();
    return true;
}

} // namespace gmatch 