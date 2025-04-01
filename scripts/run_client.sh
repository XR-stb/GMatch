#!/bin/bash

# 客户端运行脚本

# 设置颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# 默认参数
CLIENT_BIN="./build/bin/match_client_app"
SERVER_ADDRESS="127.0.0.1"
SERVER_PORT=8080

# 处理命令行参数
while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        --bin)
            CLIENT_BIN="$2"
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
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --bin FILE        Client binary path (default: ./build/bin/match_client_app)"
            echo "  --address ADDR    Server address (default: 127.0.0.1)"
            echo "  --port PORT       Server port (default: 8080)"
            echo "  --help            Display this help message"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $key${NC}"
            exit 1
            ;;
    esac
done

# 检查客户端可执行文件是否存在
if [ ! -f "$CLIENT_BIN" ]; then
    echo -e "${RED}Client binary not found: $CLIENT_BIN${NC}"
    echo -e "${YELLOW}Run ./scripts/build.sh first to build the client${NC}"
    exit 1
fi

# 显示运行配置
echo -e "${GREEN}Starting GMatch client with configuration:${NC}"
echo "  Client binary: $CLIENT_BIN"
echo "  Server address: $SERVER_ADDRESS"
echo "  Server port: $SERVER_PORT"
echo ""

# 运行客户端
echo -e "${GREEN}Starting client...${NC}"
"$CLIENT_BIN" "$SERVER_ADDRESS" "$SERVER_PORT"

# 检查是否成功运行
if [ $? -ne 0 ]; then
    echo -e "${RED}Client failed to start or closed with error${NC}"
    exit 1
fi

echo -e "${GREEN}Client closed successfully${NC}" 