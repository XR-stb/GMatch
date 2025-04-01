// 匹配队列
class MatchQueue {
public:
    MatchQueue();
    void addPlayer(const PlayerPtr& player);
    void removePlayer(Player::PlayerId playerId);
    bool tryMatchPlayers(std::vector<PlayerPtr>& matchedPlayers, int requiredPlayers, 
                        bool forceMatchOnTimeout = false, uint64_t timeoutThreshold = 5000);
    size_t size() const;
    
    void setMatchStrategy(std::shared_ptr<MatchStrategy> strategy);
    std::shared_ptr<MatchStrategy> getMatchStrategy() const;
    void clear();
}; 