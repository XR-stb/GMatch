# GMatch - 游戏匹配服务

GMatch是一个使用C++17实现的高性能游戏匹配服务。它提供基于玩家评分的匹配算法，支持客户端-服务器架构，可以应用于各种游戏类型。

## 功能特性

- 基于评分的匹配算法
- 可配置的匹配参数（评分差异、每房间人数等）
- 多线程匹配处理
- TCP服务器，支持多客户端连接
- JSON格式的通信协议
- 完整的日志系统
- 配置文件支持

## 系统要求

- C++17兼容的编译器（GCC 7+，Clang 5+）
- CMake 3.10+
- Linux/Unix系统（目前不支持Windows）
- pthread支持

## 构建与安装

### 使用构建脚本（推荐）

```bash
# 克隆仓库
git clone https://github.com/yourusername/GMatch.git
cd GMatch

# 使用构建脚本构建
./scripts/build.sh

# 可选参数
./scripts/build.sh --debug        # 以Debug模式构建
./scripts/build.sh --clean        # 清理后构建
./scripts/build.sh --no-tests     # 跳过测试
./scripts/build.sh --install      # 构建并安装
```

### 手动构建

```bash
mkdir build && cd build
cmake ..
make
```

## 运行服务器

服务器可以使用以下方式启动：

```bash
# 使用脚本（推荐）
./scripts/run_server.sh

# 自定义参数
./scripts/run_server.sh --port 8888 --log-level 0

# 手动运行
./build/match_server
```

## 运行客户端

提供了一个简单的客户端用于测试：

```bash
# 使用脚本
./scripts/run_client.sh

# 自定义服务器地址和端口
./scripts/run_client.sh --address 192.168.1.100 --port 8888

# 手动运行
./build/match_client_app 127.0.0.1 8080
```

## 运行测试

GMatch包含单元测试和集成测试：

```bash
# 运行单元测试
cd build && ctest

# 运行集成测试（模拟多个客户端）
./scripts/test_match.sh

# 自定义测试参数
./scripts/test_match.sh --clients 20 --duration 60
```

## 配置

服务器支持使用配置文件（默认为`config.ini`）进行配置：

```ini
# 服务器地址和端口
address = 0.0.0.0
port = 8080

# 匹配参数
players_per_room = 2
max_rating_diff = 300

# 日志配置
log_file = match_server.log
log_level = 1  # 0=DEBUG, 1=INFO, 2=WARNING, 3=ERROR, 4=FATAL
```

## 客户端命令

测试客户端支持以下命令：

- `create <name> <rating>` - 创建玩家
- `join` - 加入匹配队列
- `leave` - 离开匹配队列
- `rooms` - 获取房间列表
- `info` - 获取玩家信息
- `queue` - 获取队列状态
- `exit` - 退出客户端

## 项目结构

```
GMatch/
├── src/                  # 源代码
│   ├── core/             # 核心匹配逻辑
│   ├── util/             # 工具类
│   ├── server/           # 服务器实现
│   ├── client/           # 客户端实现
│   └── main.cpp          # 服务器入口点
├── test/                 # 测试代码
├── scripts/              # 脚本工具
├── build/                # 构建输出目录
├── CMakeLists.txt        # CMake配置
└── README.md             # 项目说明
```

## 协议

本项目使用MIT协议。详见LICENSE文件。

## 贡献

欢迎提交问题和拉取请求。在贡献代码前，请确保符合项目的代码风格并通过所有测试。

## 未来计划

- 支持Windows平台
- 添加Web管理界面
- 支持更多匹配算法
- 实现比赛历史记录功能 