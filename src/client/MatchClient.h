#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <deque>
#include <condition_variable>
#include "../core/Player.h"
#include "../core/Room.h"

namespace gmatch {

// 客户端事件类型
enum class ClientEventType {
    CONNECTED,
    DISCONNECTED,
    PLAYER_CREATED,
    JOINED_QUEUE,
    LEFT_QUEUE,
    MATCH_FOUND,
    ERROR
};

// 客户端事件
struct ClientEvent {
    ClientEventType type;
    std::string message;
    std::string data;
};

// 匹配客户端
class MatchClient {
public:
    using EventCallback = std::function<void(const ClientEvent&)>;
    
    MatchClient();
    ~MatchClient();
    
    // 连接服务器
    bool connect(const std::string& address, uint16_t port);
    
    // 断开连接
    void disconnect();
    
    // 创建玩家
    bool createPlayer(const std::string& name, int rating = 1500);
    
    // 加入匹配队列
    bool joinMatchmaking();
    
    // 离开匹配队列
    bool leaveMatchmaking();
    
    // 获取房间列表
    bool getRooms();
    
    // 获取玩家信息
    bool getPlayerInfo();
    
    // 获取队列状态
    bool getQueueStatus();
    
    // 设置事件回调
    void setEventCallback(EventCallback callback);
    
    // 是否已连接
    bool isConnected() const;
    
    // 获取玩家ID
    Player::PlayerId getPlayerId() const;
    
    // 获取房间ID（匹配成功后）
    Room::RoomId getRoomId() const;
    
private:
    void processEvents();
    void messageReceived(const std::string& message);
    void processResponse(const std::string& response);
    bool sendRequest(const std::string& cmd, const std::string& data);
    
    int socketFd_ = -1;
    std::atomic<bool> connected_{false};
    std::atomic<bool> running_{false};
    
    std::thread receiveThread_;
    
    Player::PlayerId playerId_ = 0;
    Room::RoomId roomId_ = 0;
    
    EventCallback eventCallback_;
    std::mutex callbackMutex_;
    
    std::deque<ClientEvent> eventQueue_;
    std::mutex eventQueueMutex_;
    std::condition_variable eventQueueCv_;
    std::thread eventProcessThread_;
};

} // namespace gmatch 