#!/bin/bash

# 服务器运行脚本

# 设置颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# 默认参数
SERVER_BIN="./build/bin/match_server"
CONFIG_FILE="config.ini"
ADDRESS="0.0.0.0"
PORT=8080
LOG_FILE="match_server.log"
LOG_LEVEL=1  # INFO

# 处理命令行参数
while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        --bin)
            SERVER_BIN="$2"
            shift 2
            ;;
        --config)
            CONFIG_FILE="$2"
            shift 2
            ;;
        --address)
            ADDRESS="$2"
            shift 2
            ;;
        --port)
            PORT="$2"
            shift 2
            ;;
        --log-file)
            LOG_FILE="$2"
            shift 2
            ;;
        --log-level)
            LOG_LEVEL="$2"
            shift 2
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --bin FILE        Server binary path (default: ./build/bin/match_server)"
            echo "  --config FILE     Config file path (default: config.ini)"
            echo "  --address ADDR    Server address (default: 0.0.0.0)"
            echo "  --port PORT       Server port (default: 8080)"
            echo "  --log-file FILE   Log file path (default: match_server.log)"
            echo "  --log-level LEVEL Log level (0=DEBUG, 1=INFO, 2=WARNING, 3=ERROR, 4=FATAL) (default: 1)"
            echo "  --help            Display this help message"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $key${NC}"
            exit 1
            ;;
    esac
done

# 检查服务器可执行文件是否存在
if [ ! -f "$SERVER_BIN" ]; then
    echo -e "${RED}Server binary not found: $SERVER_BIN${NC}"
    echo -e "${YELLOW}Run ./scripts/build.sh first to build the server${NC}"
    exit 1
fi

# 创建配置文件
if [ ! -f "$CONFIG_FILE" ]; then
    echo -e "${YELLOW}Creating default config file: $CONFIG_FILE${NC}"
    cat > "$CONFIG_FILE" << EOF
# GMatch Configuration File
address = $ADDRESS
port = $PORT
players_per_room = 2
max_rating_diff = 300
log_file = $LOG_FILE
log_level = $LOG_LEVEL
EOF
fi

# 显示运行配置
echo -e "${GREEN}Starting GMatch server with configuration:${NC}"
echo "  Server binary: $SERVER_BIN"
echo "  Config file: $CONFIG_FILE"
echo "  Address: $ADDRESS"
echo "  Port: $PORT"
echo "  Log file: $LOG_FILE"
echo "  Log level: $LOG_LEVEL"
echo ""

# 运行服务器
echo -e "${GREEN}Starting server...${NC}"
"$SERVER_BIN"

# 检查是否成功运行
if [ $? -ne 0 ]; then
    echo -e "${RED}Server failed to start${NC}"
    exit 1
fi 