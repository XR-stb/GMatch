set(CLIENT_SOURCES
    MatchClient.cpp
)

add_library(match_client ${CLIENT_SOURCES})
target_link_libraries(match_client match_util)

# 客户端可执行文件
add_executable(match_client_app SimpleClient.cpp)
target_link_libraries(match_client_app 
    match_client
    match_util
    ${CMAKE_THREAD_LIBS_INIT}
) 