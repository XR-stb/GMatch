#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "../src/core/MatchMaker.h"

using namespace gmatch;

class MatchMakerTest : public ::testing::Test {
protected:
    void SetUp() override {
        matchMaker = std::make_shared<MatchMaker>(2);
    }
    
    void TearDown() override {
        matchMaker->stop();
    }
    
    std::shared_ptr<MatchMaker> matchMaker;
};

TEST_F(MatchMakerTest, AddRemovePlayer) {
    auto player1 = std::make_shared<Player>(1, "Player1", 1500);
    auto player2 = std::make_shared<Player>(2, "Player2", 1600);
    
    matchMaker->addPlayer(player1);
    matchMaker->addPlayer(player2);
    
    // 没有好的方法直接检查队列内容，所以只检查队列大小
    EXPECT_EQ(matchMaker->getQueueSize(), 2);
    
    matchMaker->removePlayer(1);
    EXPECT_EQ(matchMaker->getQueueSize(), 1);
    
    matchMaker->removePlayer(2);
    EXPECT_EQ(matchMaker->getQueueSize(), 0);
}

TEST_F(MatchMakerTest, MatchPlayers) {
    auto player1 = std::make_shared<Player>(1, "Player1", 1500);
    auto player2 = std::make_shared<Player>(2, "Player2", 1600);
    
    // 启动匹配服务
    matchMaker->start();
    
    // 添加玩家到队列
    matchMaker->addPlayer(player1);
    matchMaker->addPlayer(player2);
    
    // 等待匹配完成
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // 检查队列应该为空（玩家已被匹配）
    EXPECT_EQ(matchMaker->getQueueSize(), 0);
    
    // 检查是否创建了房间
    auto rooms = matchMaker->getRooms();
    EXPECT_GE(rooms.size(), 1);
    
    // 检查房间中是否有我们的玩家
    bool foundPlayer1 = false;
    bool foundPlayer2 = false;
    
    for (const auto& room : rooms) {
        auto players = room->getPlayers();
        for (const auto& player : players) {
            if (player->getId() == 1) {
                foundPlayer1 = true;
            } else if (player->getId() == 2) {
                foundPlayer2 = true;
            }
        }
    }
    
    EXPECT_TRUE(foundPlayer1);
    EXPECT_TRUE(foundPlayer2);
}

TEST_F(MatchMakerTest, RatingBasedMatching) {
    auto player1 = std::make_shared<Player>(1, "Player1", 1500);
    auto player2 = std::make_shared<Player>(2, "Player2", 2000); // 评分差太大
    
    // 限制最大评分差为300
    auto strategy = std::make_shared<RatingBasedStrategy>(300);
    matchMaker->setMatchStrategy(strategy);
    
    // 启动匹配服务
    matchMaker->start();
    
    // 添加玩家到队列
    matchMaker->addPlayer(player1);
    matchMaker->addPlayer(player2);
    
    // 等待一段时间
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // 由于评分差异太大，应该不会匹配成功，队列中应该仍有2个玩家
    EXPECT_EQ(matchMaker->getQueueSize(), 2);
    
    // 没有创建房间
    auto rooms = matchMaker->getRooms();
    EXPECT_EQ(rooms.size(), 0);
    
    // 添加一个评分接近的玩家
    auto player3 = std::make_shared<Player>(3, "Player3", 1600);
    matchMaker->addPlayer(player3);
    
    // 等待匹配完成
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // 应该匹配player1和player3，队列中只剩下player2
    EXPECT_EQ(matchMaker->getQueueSize(), 1);
    
    // 检查是否创建了房间
    rooms = matchMaker->getRooms();
    EXPECT_GE(rooms.size(), 1);
    
    // 检查房间中是否有player1和player3
    bool foundPlayer1 = false;
    bool foundPlayer3 = false;
    
    for (const auto& room : rooms) {
        auto players = room->getPlayers();
        for (const auto& player : players) {
            if (player->getId() == 1) {
                foundPlayer1 = true;
            } else if (player->getId() == 3) {
                foundPlayer3 = true;
            }
        }
    }
    
    EXPECT_TRUE(foundPlayer1);
    EXPECT_TRUE(foundPlayer3);
} 