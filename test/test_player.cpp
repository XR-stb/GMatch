#include <gtest/gtest.h>
#include "../src/core/Player.h"

using namespace gmatch;

TEST(PlayerTest, Constructor) {
    Player player(1, "TestPlayer", 1500);
    
    EXPECT_EQ(player.getId(), 1);
    EXPECT_EQ(player.getName(), "TestPlayer");
    EXPECT_EQ(player.getRating(), 1500);
    EXPECT_FALSE(player.isInQueue());
}

TEST(PlayerTest, SetRating) {
    Player player(1, "TestPlayer", 1500);
    
    player.setRating(1600);
    EXPECT_EQ(player.getRating(), 1600);
}

TEST(PlayerTest, SetStatus) {
    Player player(1, "TestPlayer", 1500);
    
    EXPECT_FALSE(player.isInQueue());
    
    player.setStatus(true);
    EXPECT_TRUE(player.isInQueue());
    
    player.setStatus(false);
    EXPECT_FALSE(player.isInQueue());
}

TEST(PlayerTest, UpdateActivity) {
    Player player(1, "TestPlayer", 1500);
    
    uint64_t timestamp = 123456789;
    player.updateActivity(timestamp);
    EXPECT_EQ(player.getLastActivityTime(), timestamp);
} 