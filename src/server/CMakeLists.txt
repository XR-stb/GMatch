set(SERVER_SOURCES
    MatchServer.cpp
    TcpServer.cpp
    RequestHandler.cpp
)

add_library(match_server_lib ${SERVER_SOURCES})
target_link_libraries(match_server_lib 
    match_core
    match_util
) 