#include <gtest/gtest.h>
#include "../src/core/Room.h"

using namespace gmatch;

TEST(RoomTest, Constructor) {
    Room room(1, 4, 1000, 2000);
    
    EXPECT_EQ(room.getId(), 1);
    EXPECT_EQ(room.getCapacity(), 4);
    EXPECT_EQ(room.getStatus(), Room::Status::WAITING);
    EXPECT_EQ(room.getPlayerCount(), 0);
    EXPECT_FALSE(room.isFull());
}

TEST(RoomTest, AddPlayer) {
    Room room(1, 2);
    
    auto player1 = std::make_shared<Player>(1, "Player1", 1500);
    auto player2 = std::make_shared<Player>(2, "Player2", 1600);
    auto player3 = std::make_shared<Player>(3, "Player3", 1700);
    
    EXPECT_TRUE(room.addPlayer(player1));
    EXPECT_EQ(room.getPlayerCount(), 1);
    EXPECT_FALSE(room.isFull());
    EXPECT_EQ(room.getStatus(), Room::Status::WAITING);
    
    EXPECT_TRUE(room.addPlayer(player2));
    EXPECT_EQ(room.getPlayerCount(), 2);
    EXPECT_TRUE(room.isFull());
    EXPECT_EQ(room.getStatus(), Room::Status::READY);
    
    // 房间已满，不能再添加玩家
    EXPECT_FALSE(room.addPlayer(player3));
    EXPECT_EQ(room.getPlayerCount(), 2);
}

TEST(RoomTest, RemovePlayer) {
    Room room(1, 2);
    
    auto player1 = std::make_shared<Player>(1, "Player1", 1500);
    auto player2 = std::make_shared<Player>(2, "Player2", 1600);
    
    room.addPlayer(player1);
    room.addPlayer(player2);
    
    EXPECT_EQ(room.getStatus(), Room::Status::READY);
    
    EXPECT_TRUE(room.removePlayer(1));
    EXPECT_EQ(room.getPlayerCount(), 1);
    EXPECT_FALSE(room.isFull());
    EXPECT_EQ(room.getStatus(), Room::Status::WAITING);
    
    // 玩家已经移除，再次移除应该失败
    EXPECT_FALSE(room.removePlayer(1));
    
    EXPECT_TRUE(room.removePlayer(2));
    EXPECT_EQ(room.getPlayerCount(), 0);
}

TEST(RoomTest, RatingRange) {
    Room room(1, 2, 1200, 1800);
    
    auto player1 = std::make_shared<Player>(1, "Player1", 1500);
    auto player2 = std::make_shared<Player>(2, "Player2", 1100);
    auto player3 = std::make_shared<Player>(3, "Player3", 1900);
    
    // 玩家1在评分范围内
    EXPECT_TRUE(room.isRatingInRange(player1->getRating()));
    EXPECT_TRUE(room.addPlayer(player1));
    
    // 玩家2评分过低
    EXPECT_FALSE(room.isRatingInRange(player2->getRating()));
    EXPECT_FALSE(room.addPlayer(player2));
    
    // 玩家3评分过高
    EXPECT_FALSE(room.isRatingInRange(player3->getRating()));
    EXPECT_FALSE(room.addPlayer(player3));
}

TEST(RoomTest, GetAverageRating) {
    Room room(1, 3);
    
    auto player1 = std::make_shared<Player>(1, "Player1", 1500);
    auto player2 = std::make_shared<Player>(2, "Player2", 1700);
    
    // 空房间
    EXPECT_DOUBLE_EQ(room.getAverageRating(), 0.0);
    
    room.addPlayer(player1);
    EXPECT_DOUBLE_EQ(room.getAverageRating(), 1500.0);
    
    room.addPlayer(player2);
    EXPECT_DOUBLE_EQ(room.getAverageRating(), 1600.0);
}

TEST(RoomTest, GetPlayers) {
    Room room(1, 2);
    
    auto player1 = std::make_shared<Player>(1, "Player1", 1500);
    auto player2 = std::make_shared<Player>(2, "Player2", 1700);
    
    room.addPlayer(player1);
    room.addPlayer(player2);
    
    auto players = room.getPlayers();
    EXPECT_EQ(players.size(), 2);
    
    // 验证返回的玩家列表包含添加的玩家
    bool foundPlayer1 = false;
    bool foundPlayer2 = false;
    
    for (const auto& player : players) {
        if (player->getId() == 1) {
            foundPlayer1 = true;
        } else if (player->getId() == 2) {
            foundPlayer2 = true;
        }
    }
    
    EXPECT_TRUE(foundPlayer1);
    EXPECT_TRUE(foundPlayer2);
} 