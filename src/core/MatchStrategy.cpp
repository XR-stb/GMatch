#include "MatchStrategy.h"
#include <cmath>

namespace gmatch {

// RatingBasedStrategy 实现
RatingBasedStrategy::RatingBasedStrategy(int maxRatingDiff)
    : maxRatingDiff_(maxRatingDiff) {
}

bool RatingBasedStrategy::isMatch(const PlayerPtr& player1, const PlayerPtr& player2) const {
    int ratingDiff = std::abs(player1->getRating() - player2->getRating());
    return ratingDiff <= maxRatingDiff_;
}

} // namespace gmatch 