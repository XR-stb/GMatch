#!/bin/bash

# 匹配系统集成测试脚本

# 设置颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# 默认参数
SERVER_BIN="./build/bin/match_server"
CLIENT_BIN="./build/bin/match_client_app"
NUM_CLIENTS=10
SERVER_ADDRESS="127.0.0.1"
SERVER_PORT=8080
TEST_DURATION=30
VERBOSE=false

# 处理命令行参数
while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        --server-bin)
            SERVER_BIN="$2"
            shift 2
            ;;
        --client-bin)
            CLIENT_BIN="$2"
            shift 2
            ;;
        --clients)
            NUM_CLIENTS="$2"
            shift 2
            ;;
        --address)
            SERVER_ADDRESS="$2"
            shift 2
            ;;
        --port)
            SERVER_PORT="$2"
            shift 2
            ;;
        --duration)
            TEST_DURATION="$2"
            shift 2
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --server-bin FILE Server binary path (default: ./build/bin/match_server)"
            echo "  --client-bin FILE Client binary path (default: ./build/bin/match_client_app)"
            echo "  --clients N       Number of clients to simulate (default: 10)"
            echo "  --address ADDR    Server address (default: 127.0.0.1)"
            echo "  --port PORT       Server port (default: 8080)"
            echo "  --duration SEC    Test duration in seconds (default: 30)"
            echo "  --verbose         Enable verbose output"
            echo "  --help            Display this help message"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $key${NC}"
            exit 1
            ;;
    esac
done

# 检查可执行文件是否存在
if [ ! -f "$SERVER_BIN" ]; then
    echo -e "${RED}Server binary not found: $SERVER_BIN${NC}"
    echo -e "${YELLOW}Run ./scripts/build.sh first to build the server${NC}"
    exit 1
fi

if [ ! -f "$CLIENT_BIN" ]; then
    echo -e "${RED}Client binary not found: $CLIENT_BIN${NC}"
    echo -e "${YELLOW}Run ./scripts/build.sh first to build the client${NC}"
    exit 1
fi

# 确保测试目录存在
TEMP_DIR="test_output"
mkdir -p "$TEMP_DIR"

# 显示测试配置
echo -e "${GREEN}Starting GMatch integration test with configuration:${NC}"
echo "  Server binary: $SERVER_BIN"
echo "  Client binary: $CLIENT_BIN"
echo "  Number of clients: $NUM_CLIENTS"
echo "  Server address: $SERVER_ADDRESS"
echo "  Server port: $SERVER_PORT"
echo "  Test duration: $TEST_DURATION seconds"
echo ""

# 创建测试配置
CONFIG_FILE="$TEMP_DIR/test_config.ini"
cat > "$CONFIG_FILE" << EOF
# GMatch Test Configuration File
address = $SERVER_ADDRESS
port = $SERVER_PORT
players_per_room = 2
max_rating_diff = 500
log_file = $TEMP_DIR/test_server.log
log_level = 0
EOF

# 启动服务器
echo -e "${GREEN}Starting server...${NC}"
LOG_FILE="$TEMP_DIR/server_output.log"
$SERVER_BIN > "$LOG_FILE" 2>&1 &
SERVER_PID=$!

# 等待服务器启动
echo -e "${YELLOW}Waiting for server to start...${NC}"
sleep 2

# 检查服务器是否正在运行
if ! ps -p $SERVER_PID > /dev/null; then
    echo -e "${RED}Server failed to start. Check $LOG_FILE for details${NC}"
    exit 1
fi

# 客户端测试函数
run_client_test() {
    local client_id=$1
    local client_log="$TEMP_DIR/client_${client_id}.log"
    local client_input="$TEMP_DIR/client_${client_id}.in"
    
    # 生成客户端输入文件
    cat > "$client_input" << EOF
create Player${client_id} $(( 1000 + RANDOM % 1000 ))
join
EOF
    
    # 在后台运行客户端并将输入文件传递给它
    $CLIENT_BIN $SERVER_ADDRESS $SERVER_PORT < "$client_input" > "$client_log" 2>&1 &
    echo $!  # 返回客户端PID
}

# 启动客户端
echo -e "${GREEN}Starting $NUM_CLIENTS clients...${NC}"
CLIENT_PIDS=()
for ((i=1; i<=$NUM_CLIENTS; i++)); do
    if [ "$VERBOSE" = true ]; then
        echo -e "${YELLOW}Starting client $i...${NC}"
    fi
    
    client_pid=$(run_client_test $i)
    CLIENT_PIDS+=($client_pid)
    
    # 随机延迟，使得客户端不会同时进入
    sleep $(bc -l <<< "scale=2; $RANDOM/32767")
done

# 等待测试持续时间
echo -e "${YELLOW}Test running for $TEST_DURATION seconds...${NC}"
sleep $TEST_DURATION

# 停止所有客户端
echo -e "${GREEN}Stopping clients...${NC}"
for pid in "${CLIENT_PIDS[@]}"; do
    if ps -p $pid > /dev/null; then
        kill -TERM $pid 2>/dev/null
    fi
done

# 停止服务器
echo -e "${GREEN}Stopping server...${NC}"
kill -TERM $SERVER_PID 2>/dev/null

# 等待服务器完全停止
wait $SERVER_PID 2>/dev/null

# 分析日志
echo -e "${GREEN}Analyzing logs...${NC}"
MATCH_COUNT=$(grep -c "Match found!" "$LOG_FILE")
PLAYER_COUNT=$(grep -c "Player created successfully" "$LOG_FILE")

echo -e "${GREEN}Test results:${NC}"
echo "  Players created: $PLAYER_COUNT"
echo "  Matches completed: $MATCH_COUNT"

if [ "$PLAYER_COUNT" -eq "$NUM_CLIENTS" ] && [ "$MATCH_COUNT" -gt 0 ]; then
    echo -e "${GREEN}Test passed!${NC}"
    exit 0
else
    echo -e "${RED}Test failed!${NC}"
    echo "Expected $NUM_CLIENTS players and at least one match."
    echo "Check logs in $TEMP_DIR directory for details."
    exit 1
fi 