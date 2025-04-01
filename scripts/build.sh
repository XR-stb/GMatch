#!/bin/bash

# 构建脚本

# 设置颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 默认参数
BUILD_TYPE="Release"
BUILD_DIR="build"
CLEAN_BUILD=false
RUN_TESTS=true
INSTALL=false
INSTALL_PREFIX="/usr/local"
PARALLEL_JOBS=$(nproc)

# 处理命令行参数
while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        --no-tests)
            RUN_TESTS=false
            shift
            ;;
        --install)
            INSTALL=true
            shift
            ;;
        --install-prefix=*)
            INSTALL_PREFIX="${1#*=}"
            shift
            ;;
        --jobs=*)
            PARALLEL_JOBS="${1#*=}"
            shift
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --debug          Build in Debug mode (default: Release)"
            echo "  --build-dir DIR  Set build directory (default: build)"
            echo "  --clean          Clean build directory before building"
            echo "  --verbose        Enable verbose output"
            echo "  --no-tests       Skip running tests"
            echo "  --install        Install after building"
            echo "  --install-prefix=DIR  Install directory (default: /usr/local)"
            echo "  --jobs=N         Number of parallel jobs (default: $(nproc))"
            echo "  --help           Display this help message"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown parameter: $key${NC}"
            exit 1
            ;;
    esac
done

# 显示构建配置
echo -e "${GREEN}Building GMatch with configuration:${NC}"
echo "  Build type: $BUILD_TYPE"
echo "  Build directory: $BUILD_DIR"
echo "  Clean build: $CLEAN_BUILD"
echo "  Run tests: $RUN_TESTS"
echo "  Install: $INSTALL"
if [ "$INSTALL" = true ]; then
    echo "  Install prefix: $INSTALL_PREFIX"
fi
echo "  Parallel jobs: $PARALLEL_JOBS"
echo ""

# 创建或清理构建目录
if [ "$CLEAN_BUILD" = true ] && [ -d "$BUILD_DIR" ]; then
    echo -e "${BLUE}Cleaning build directory...${NC}"
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"

# 获取项目根目录
PROJECT_ROOT=$(pwd)

cd "$BUILD_DIR" || { echo -e "${RED}Failed to enter build directory${NC}"; exit 1; }

# 运行 CMake 配置
echo -e "${BLUE}Configuring project...${NC}"
cmake_args=("-DCMAKE_BUILD_TYPE=$BUILD_TYPE")

if [ "$INSTALL" = true ]; then
    cmake_args+=("-DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX")
fi

if [ "$VERBOSE" = true ]; then
    cmake .. "${cmake_args[@]}" -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON
else
    cmake .. "${cmake_args[@]}"
fi

# 检查 CMake 是否成功
if [ $? -ne 0 ]; then
    echo -e "${RED}CMake configuration failed${NC}"
    exit 1
fi

# 构建项目
echo -e "${BLUE}Building project...${NC}"
if [ "$VERBOSE" = true ]; then
    cmake --build . --config "$BUILD_TYPE" --parallel "$PARALLEL_JOBS" --verbose
else
    cmake --build . --config "$BUILD_TYPE" --parallel "$PARALLEL_JOBS"
fi

# 检查构建是否成功
if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed${NC}"
    exit 1
fi

echo -e "${GREEN}Build successful${NC}"

# 运行测试
if [ "$RUN_TESTS" = true ]; then
    echo -e "${BLUE}Running tests...${NC}"
    ctest --output-on-failure
    
    # 检查测试是否成功
    if [ $? -ne 0 ]; then
        echo -e "${RED}Tests failed${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}All tests passed${NC}"
fi

# 安装
if [ "$INSTALL" = true ]; then
    echo -e "${BLUE}Installing to $INSTALL_PREFIX...${NC}"
    
    # 检查是否需要sudo权限
    if [[ "$INSTALL_PREFIX" == "/usr/local"* || "$INSTALL_PREFIX" == "/usr"* ]]; then
        if [ $(id -u) -ne 0 ]; then
            echo -e "${YELLOW}Installing to $INSTALL_PREFIX requires root privileges.${NC}"
            echo -e "${YELLOW}Using sudo to install...${NC}"
            sudo cmake --install .
        else
            cmake --install .
        fi
    else
        # 用户指定的目录，尝试创建
        mkdir -p "$INSTALL_PREFIX"
        cmake --install .
    fi
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}Installation failed${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}Installation completed successfully${NC}"
fi

# 返回项目根目录
cd "$PROJECT_ROOT" || { echo -e "${RED}Failed to return to project root${NC}"; exit 1; }

echo -e "${GREEN}All done!${NC}" 