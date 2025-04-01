#pragma once

#include <memory>
#include "Player.h"

namespace gmatch {

// 匹配策略接口
class MatchStrategy {
public:
    virtual ~MatchStrategy() = default;
    virtual bool isMatch(const PlayerPtr& player1, const PlayerPtr& player2) const = 0;
};

// 基于评分差异的匹配策略
class RatingBasedStrategy : public MatchStrategy {
public:
    RatingBasedStrategy(int maxRatingDiff = 300);
    bool isMatch(const PlayerPtr& player1, const PlayerPtr& player2) const override;
    
    // 获取最大评分差异
    int getMaxRatingDiff() const { return maxRatingDiff_; }
    
private:
    int maxRatingDiff_;
};

} // namespace gmatch 