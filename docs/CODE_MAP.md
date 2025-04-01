# GMatch 代码导图

本文档提供了GMatch项目代码的组织结构和主要文件的说明，帮助新开发者快速了解项目架构和代码关系。

## 目录结构

```
GMatch/
├── src/                  # 源代码目录
│   ├── core/             # 核心匹配逻辑
│   │   ├── Player.h      # 玩家类定义
│   │   ├── Player.cpp    # 玩家类实现
│   │   ├── Room.h        # 房间类定义
│   │   ├── Room.cpp      # 房间类实现
│   │   ├── MatchMaker.h  # 匹配器类定义
│   │   ├── MatchMaker.cpp # 匹配器类实现
│   │   ├── MatchManager.h # 匹配管理器类定义
│   │   └── MatchManager.cpp # 匹配管理器类实现
│   ├── util/             # 工具类
│   │   ├── Logger.h      # 日志类定义
│   │   ├── Logger.cpp    # 日志类实现
│   │   ├── Config.h      # 配置类定义
│   │   ├── Config.cpp    # 配置类实现
│   │   ├── Utils.h       # 通用工具函数
│   │   └── Utils.cpp     # 通用工具函数实现
│   ├── server/           # 服务器实现
│   │   ├── RequestHandler.h    # 请求处理器定义
│   │   ├── RequestHandler.cpp  # 请求处理器实现
│   │   ├── MatchServer.h       # 匹配服务器定义
│   │   └── MatchServer.cpp     # 匹配服务器实现
│   ├── client/           # 客户端实现
│   │   ├── MatchClient.h        # 匹配客户端定义
│   │   ├── MatchClient.cpp      # 匹配客户端实现
│   │   ├── MatchClientApp.cpp   # 客户端应用程序入口
│   │   └── MatchClientEvents.h  # 客户端事件定义
│   ├── main.cpp          # 服务器入口点
│   └── CMakeLists.txt    # 源代码CMake配置
├── test/                 # 测试代码
│   ├── PlayerTest.cpp    # 玩家类测试
│   ├── RoomTest.cpp      # 房间类测试
│   ├── MatchMakerTest.cpp # 匹配器类测试
│   ├── MatchManagerTest.cpp # 匹配管理器类测试
│   └── CMakeLists.txt    # 测试CMake配置
├── scripts/              # 脚本工具
│   ├── build.sh          # 构建脚本
│   ├── run_server.sh     # 服务器运行脚本
│   ├── run_client.sh     # 客户端运行脚本
│   └── test_match.sh     # 自动化测试脚本
├── docs/                 # 文档
│   ├── DESIGN.md         # 设计文档
│   └── CODE_MAP.md       # 代码导图（本文件）
├── build/                # 构建输出目录
├── CMakeLists.txt        # 顶层CMake配置
├── config.ini.example    # 配置文件示例
├── LICENSE               # 许可证文件
├── README.md             # 项目说明
└── CONTRIBUTING.md       # 贡献指南
```

## 核心类关系

### 匹配系统组件关系

```
    +-------------+
    |             |
    | MatchManager|
    |             |
    +------+------+
           |
           | 使用
           v
    +------+------+     管理    +-------------+
    |             +------------>+             |
    |  MatchMaker |             |    Room     |
    |             |             |             |
    +------+------+             +------+------+
           |                           |
           | 匹配                      | 包含
           v                           v
    +------+------+             +------+------+
    |             |             |             |
    |   Player    |<------------+   Player    |
    |             |   加入      |             |
    +-------------+             +-------------+
```

### 服务器组件关系

```
    +-------------+
    |             |
    | MatchServer |
    |             |
    +------+------+
           |
           | 处理请求
           v
    +------+------+     使用     +-------------+
    |             +------------>+             |
    |RequestHandler|            | MatchManager|
    |             |             |             |
    +-------------+             +-------------+
```

### 客户端组件关系

```
    +-------------+
    |             |
    |MatchClientApp|
    |             |
    +------+------+
           |
           | 使用
           v
    +------+------+     触发     +-------------+
    |             +------------>+             |
    | MatchClient |            |ClientEventHandler|
    |             |             |             |
    +-------------+             +-------------+
```

## 主要文件说明

### 核心匹配逻辑

- **Player.h/cpp**: 玩家类，表示一个游戏玩家，包含玩家基本信息和状态
- **Room.h/cpp**: 房间类，管理一组玩家，处理玩家加入/离开
- **MatchMaker.h/cpp**: 匹配器类，实现玩家匹配算法，管理匹配队列
- **MatchManager.h/cpp**: 匹配管理器类，协调整个匹配过程，对外提供接口

### 服务器实现

- **RequestHandler.h/cpp**: 请求处理器，解析客户端请求，执行相应操作
- **MatchServer.h/cpp**: 匹配服务器，处理网络通信，管理客户端连接
- **main.cpp**: 服务器启动入口，配置和初始化服务器

### 客户端实现

- **MatchClient.h/cpp**: 匹配客户端库，提供与服务器交互的API
- **MatchClientApp.cpp**: 示例客户端应用，展示如何使用匹配客户端库
- **MatchClientEvents.h**: 客户端事件定义，用于客户端与应用之间的通信

### 工具类

- **Logger.h/cpp**: 日志类，处理日志记录和输出
- **Config.h/cpp**: 配置类，读取和解析配置文件
- **Utils.h/cpp**: 通用工具函数，包含各种辅助功能

## 代码流程

### 服务器启动流程

1. main.cpp: 读取配置文件
2. 初始化日志系统
3. 创建MatchManager实例
4. 创建RequestHandler实例
5. 创建MatchServer实例
6. 启动服务器监听连接
7. 启动匹配线程

### 客户端使用流程

1. 创建MatchClient实例
2. 连接到服务器
3. 创建玩家
4. 加入匹配队列
5. 等待匹配结果
6. 接收匹配成功事件
7. 离开队列或断开连接

### 匹配过程流程

1. 玩家通过客户端加入匹配队列
2. RequestHandler接收请求并转发给MatchManager
3. MatchManager将玩家添加到MatchMaker的队列中
4. MatchMaker周期性执行匹配算法
5. 找到合适的玩家后创建Room
6. 将匹配结果通知给相关玩家

## 如何扩展代码

### 添加新命令

1. 在RequestHandler.h中添加新的处理函数
2. 在RequestHandler.cpp的processRequest方法中添加新命令处理分支
3. 在MatchClient.h/cpp中添加相应的客户端方法

### 修改匹配算法

1. 修改MatchMaker.cpp中的matchPlayers方法
2. 可以创建新的MatchMaker子类实现不同的匹配算法

### 添加新的玩家属性

1. 修改Player.h/cpp添加新的属性
2. 更新相关的序列化/反序列化代码
3. 根据需要修改匹配算法考虑新属性

### 添加新的统计指标

1. 在MatchManager中添加统计计数器
2. 添加新的查询接口
3. 在RequestHandler中实现相应的处理函数 