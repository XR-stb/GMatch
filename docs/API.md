# GMatch API文档

本文档详细说明了GMatch匹配服务的API，包括客户端与服务器之间的通信协议和命令。

## 通信协议

GMatch使用TCP作为传输层协议，使用JSON作为消息格式。每个消息都是一个完整的JSON对象，以换行符(`\n`)结束。

### 请求格式

客户端发送到服务器的请求格式如下：

```json
{
    "cmd": "命令名称",
    "data": {
        // 命令特定的参数
    }
}
```

### 响应格式

服务器返回给客户端的响应格式如下：

```json
{
    "status": 0,      // 状态码，0表示成功，非0表示错误
    "msg": "OK",      // 状态消息，成功时为"OK"，失败时为错误描述
    "data": {
        // 命令特定的响应数据
    }
}
```

### 事件格式

服务器推送给客户端的事件格式如下：

```json
{
    "event": "事件名称",
    "data": {
        // 事件特定的数据
    }
}
```

## 状态码

| 状态码 | 描述 |
|--------|------|
| 0 | 成功 |
| 1 | 通用错误 |
| 2 | 无效的命令 |
| 3 | 无效的参数 |
| 4 | 玩家不存在 |
| 5 | 玩家已存在 |
| 6 | 玩家已在队列中 |
| 7 | 玩家不在队列中 |
| 8 | 房间不存在 |
| 9 | 服务器内部错误 |

## 命令

### 创建玩家

创建一个新的玩家。

**请求：**

```json
{
    "cmd": "create_player",
    "data": {
        "name": "玩家名称",
        "rating": 1500
    }
}
```

**参数：**

- `name`: 字符串，玩家名称
- `rating`: 整数，玩家初始评分（默认值：1500）

**响应：**

```json
{
    "status": 0,
    "msg": "OK",
    "data": {
        "player_id": "a1b2c3d4",
        "name": "玩家名称",
        "rating": 1500
    }
}
```

**错误：**

- 5: 玩家已存在（名称重复）
- 3: 无效的参数（评分范围错误或名称为空）

### 加入匹配队列

将玩家加入匹配队列。

**请求：**

```json
{
    "cmd": "join_queue",
    "data": {
        "player_id": "a1b2c3d4"
    }
}
```

**参数：**

- `player_id`: 字符串，玩家ID

**响应：**

```json
{
    "status": 0,
    "msg": "OK",
    "data": {
        "position": 3,
        "estimated_wait_time": 30
    }
}
```

**错误：**

- 4: 玩家不存在
- 6: 玩家已在队列中

### 离开匹配队列

将玩家从匹配队列中移除。

**请求：**

```json
{
    "cmd": "leave_queue",
    "data": {
        "player_id": "a1b2c3d4"
    }
}
```

**参数：**

- `player_id`: 字符串，玩家ID

**响应：**

```json
{
    "status": 0,
    "msg": "OK",
    "data": {}
}
```

**错误：**

- 4: 玩家不存在
- 7: 玩家不在队列中

### 获取房间列表

获取当前所有活跃房间的列表。

**请求：**

```json
{
    "cmd": "get_rooms",
    "data": {}
}
```

**响应：**

```json
{
    "status": 0,
    "msg": "OK",
    "data": {
        "rooms": [
            {
                "room_id": "r1s2t3u4",
                "status": "waiting",
                "player_count": 1,
                "capacity": 2,
                "players": [
                    {
                        "player_id": "a1b2c3d4",
                        "name": "玩家1",
                        "rating": 1500
                    }
                ]
            },
            {
                "room_id": "v5w6x7y8",
                "status": "matched",
                "player_count": 2,
                "capacity": 2,
                "players": [
                    {
                        "player_id": "e5f6g7h8",
                        "name": "玩家2",
                        "rating": 1600
                    },
                    {
                        "player_id": "i9j0k1l2",
                        "name": "玩家3",
                        "rating": 1550
                    }
                ]
            }
        ],
        "total_count": 2
    }
}
```

### 获取玩家信息

获取指定玩家的详细信息。

**请求：**

```json
{
    "cmd": "get_player_info",
    "data": {
        "player_id": "a1b2c3d4"
    }
}
```

**参数：**

- `player_id`: 字符串，玩家ID

**响应：**

```json
{
    "status": 0,
    "msg": "OK",
    "data": {
        "player_id": "a1b2c3d4",
        "name": "玩家名称",
        "rating": 1500,
        "status": "queuing",
        "in_room": false,
        "room_id": null,
        "create_time": "2023-05-20T12:34:56Z",
        "last_active_time": "2023-05-20T12:40:12Z"
    }
}
```

**错误：**

- 4: 玩家不存在

### 获取队列状态

获取当前匹配队列的状态。

**请求：**

```json
{
    "cmd": "get_queue_status",
    "data": {}
}
```

**响应：**

```json
{
    "status": 0,
    "msg": "OK",
    "data": {
        "queue_size": 10,
        "avg_wait_time": 45,
        "rating_distribution": [
            {
                "range": "0-1000",
                "count": 1
            },
            {
                "range": "1001-1500",
                "count": 5
            },
            {
                "range": "1501-2000",
                "count": 3
            },
            {
                "range": "2001+",
                "count": 1
            }
        ]
    }
}
```

## 事件

### 匹配成功事件

当玩家成功匹配到房间时，服务器会推送此事件。

```json
{
    "event": "match_success",
    "data": {
        "room_id": "r1s2t3u4",
        "players": [
            {
                "player_id": "a1b2c3d4",
                "name": "玩家1",
                "rating": 1500
            },
            {
                "player_id": "e5f6g7h8",
                "name": "玩家2",
                "rating": 1600
            }
        ]
    }
}
```

### 队列更新事件

当玩家在队列中的位置发生变化时，服务器会推送此事件。

```json
{
    "event": "queue_update",
    "data": {
        "player_id": "a1b2c3d4",
        "position": 2,
        "estimated_wait_time": 20
    }
}
```

### 服务器状态事件

定期推送服务器状态信息。

```json
{
    "event": "server_status",
    "data": {
        "active_players": 45,
        "queue_size": 10,
        "active_rooms": 12,
        "avg_match_time": 25
    }
}
```

## 错误处理

客户端应当处理服务器返回的错误状态，并根据错误码采取适当的措施。例如，如果服务器返回状态码4（玩家不存在），客户端应当首先创建玩家，然后再尝试其他操作。

## 重连处理

如果客户端与服务器的连接断开，客户端应当尝试重新连接。重连后，客户端可以使用之前的玩家ID获取玩家当前状态，并根据需要执行相应操作。

## 使用示例

### JavaScript示例

```javascript
const socket = new WebSocket('ws://localhost:8080');

socket.onopen = () => {
    // 创建玩家
    socket.send(JSON.stringify({
        cmd: 'create_player',
        data: {
            name: 'Player1',
            rating: 1500
        }
    }));
};

socket.onmessage = (event) => {
    const response = JSON.parse(event.data);
    
    // 处理响应
    if (response.status === 0) {
        if (response.cmd === 'create_player') {
            const playerId = response.data.player_id;
            
            // 加入队列
            socket.send(JSON.stringify({
                cmd: 'join_queue',
                data: {
                    player_id: playerId
                }
            }));
        }
    } else {
        console.error(`错误: ${response.msg}`);
    }
    
    // 处理事件
    if (response.event === 'match_success') {
        console.log('匹配成功！');
        console.log('房间ID:', response.data.room_id);
        console.log('玩家:', response.data.players);
    }
};

socket.onerror = (error) => {
    console.error('WebSocket错误:', error);
};

socket.onclose = () => {
    console.log('连接关闭，尝试重连...');
    // 实现重连逻辑
};
```

### C++示例

```cpp
#include "MatchClient.h"

int main() {
    // 创建客户端
    MatchClient client;
    
    // 连接到服务器
    client.connect("localhost", 8080);
    
    // 创建玩家
    std::string playerId = client.createPlayer("Player1", 1500);
    
    // 加入队列
    client.joinQueue(playerId);
    
    // 处理事件
    client.setEventCallback([](const std::string& event, const json& data) {
        if (event == "match_success") {
            std::cout << "匹配成功！" << std::endl;
            std::cout << "房间ID: " << data["room_id"].get<std::string>() << std::endl;
            
            // 处理匹配结果
            // ...
        }
    });
    
    // 主循环
    while (true) {
        client.update();
        // ...
    }
    
    return 0;
}
```

## 限制和注意事项

1. 消息大小不应超过64KB
2. 客户端应当实现超时和重试机制
3. 服务器可能会限制单个客户端的请求频率
4. 长时间不活动的玩家可能会被服务器自动清理 