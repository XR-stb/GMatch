set(CORE_SOURCES
    Player.cpp
    Room.cpp
    MatchManager.cpp
    MatchMaker.cpp
    MatchQueue.cpp
    MatchStrategy.cpp
)

add_library(match_core ${CORE_SOURCES})
target_include_directories(match_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(match_core match_util) 