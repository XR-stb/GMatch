set(TEST_SOURCES
    test_player.cpp
    test_room.cpp
    test_matchmaker.cpp
    test_matchmanager.cpp
)

# 添加Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

# 添加单元测试
add_executable(match_tests ${TEST_SOURCES})
target_link_libraries(match_tests 
    match_core
    match_util
    gtest
    gtest_main
    ${CMAKE_THREAD_LIBS_INIT}
)

# 添加测试
add_test(NAME match_tests COMMAND match_tests) 