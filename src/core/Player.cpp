#include "Player.h"

namespace gmatch {

Player::Player(PlayerId id, const std::string& name, int rating)
    : id_(id), name_(name), rating_(rating) {
}

} // namespace gmatch 