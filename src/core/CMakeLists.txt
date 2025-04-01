set(CORE_SOURCES
    MatchManager.cpp
    Player.cpp
    MatchMaker.cpp
    Room.cpp
)

add_library(match_core ${CORE_SOURCES})
target_link_libraries(match_core match_util) 