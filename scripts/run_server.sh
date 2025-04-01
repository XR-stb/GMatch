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
PORT=9090
LOG_FILE="match_server.log"
LOG_LEVEL=1  # INFO
FORCE_KILL=false

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
        --force-kill)
            FORCE_KILL=true
            shift
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --bin FILE        Server binary path (default: ./build/bin/match_server)"
            echo "  --config FILE     Config file path (default: config.ini)"
            echo "  --address ADDR    Server address (default: 0.0.0.0)"
            echo "  --port PORT       Server port (default: 9090)"
            echo "  --log-file FILE   Log file path (default: match_server.log)"
            echo "  --log-level LEVEL Log level (0=DEBUG, 1=INFO, 2=WARNING, 3=ERROR, 4=FATAL) (default: 1)"
            echo "  --force-kill      Kill any process using the same port before starting (default: false)"
            echo "  --help            Display this help message"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $key${NC}"
            exit 1
            ;;
    esac
done

# 检查端口是否被占用，如果占用则杀死相关进程
check_and_kill_process() {
    # 查找使用指定端口的进程
    local pid=$(lsof -t -i:$PORT 2>/dev/null)
    
    if [ -n "$pid" ]; then
        echo -e "${YELLOW}Found process (PID: $pid) using port $PORT${NC}"
        
        if [ "$FORCE_KILL" = true ]; then
            echo -e "${YELLOW}Killing process $pid...${NC}"
            kill -15 $pid 2>/dev/null
            
            # 等待进程结束
            sleep 1
            
            # 如果进程仍然存在，强制杀死
            if kill -0 $pid 2>/dev/null; then
                echo -e "${YELLOW}Process still alive, force killing...${NC}"
                kill -9 $pid 2>/dev/null
                sleep 1
            fi
            
            # 确认进程已结束
            if kill -0 $pid 2>/dev/null; then
                echo -e "${RED}Failed to kill process $pid${NC}"
                return 1
            else
                echo -e "${GREEN}Successfully killed process $pid${NC}"
                return 0
            fi
        else
            echo -e "${RED}Port $PORT is already in use by process $pid${NC}"
            echo -e "${YELLOW}Use --force-kill option to kill the process and start the server${NC}"
            return 1
        fi
    fi
    
    return 0
}

# 检查服务器可执行文件是否存在
if [ ! -f "$SERVER_BIN" ]; then
    echo -e "${RED}Server binary not found: $SERVER_BIN${NC}"
    echo -e "${YELLOW}Run ./scripts/build.sh first to build the server${NC}"
    exit 1
fi

# 检查并清理占用端口的进程
check_and_kill_process
if [ $? -ne 0 ]; then
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