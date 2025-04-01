cmake_minimum_required(VERSION 3.10)
project(GMatch VERSION 1.0)

# 设置二进制文件输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 启用compile_commands.json导出，用于clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${PROJECT_SOURCE_DIR}/src)

# 添加第三方依赖
find_package(Threads REQUIRED)

# 核心库
add_subdirectory(src/core)
add_subdirectory(src/util)
add_subdirectory(src/server)

# 可执行文件
add_executable(match_server src/main.cpp)
target_link_libraries(match_server 
    match_core
    match_util
    match_server_lib
    ${CMAKE_THREAD_LIBS_INIT}
)

# 客户端示例
add_subdirectory(src/client)

# 测试
enable_testing()
add_subdirectory(test) 