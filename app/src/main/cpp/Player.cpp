#include "Player.h"
#include <cmath>
#include <algorithm>

Player::Player(Camera &camera)
    : camera_(camera)
    , position_(0, 30, 0)
    , velocity_(0, 0, 0)
    , inputMove_(0, 0, 0) {}

void Player::move(const Vector3 &direction) {
    inputMove_ = direction;
}

void Player::jump() {
    if (onGround_) {
        velocity_.y = jumpVelocity_;
        onGround_ = false;
    }
}

bool Player::collideAndSlide(Vector3 &pos, const Vector3 &vel, float dt, ChunkManager &cm) {
    Vector3 newPos = pos + vel * dt;

    auto occupied = [&](float px, float py, float pz) -> bool {
        int bx = (int)std::floor(px);
        int by = (int)std::floor(py);
        int bz = (int)std::floor(pz);
        return cm.getBlock(bx, by, bz) != BlockType::AIR;
    };

    float hw = width_ / 2.0f;

    if (std::abs(vel.x) > 0.001f) {
        bool hit = false;
        for (float dy = 0; dy <= height_; dy += 0.5f) {
            for (float dz = -hw; dz <= hw; dz += hw * 2) {
                if (occupied(newPos.x + (vel.x > 0 ? hw : -hw), pos.y + dy, pos.z + dz)) {
                    hit = true; goto xDone;
                }
            }
        }
        xDone:
        if (!hit) pos.x = newPos.x;
        else velocity_.x = 0;
    }

    if (std::abs(vel.z) > 0.001f) {
        bool hit = false;
        for (float dy = 0; dy <= height_; dy += 0.5f) {
            for (float dx = -hw; dx <= hw; dx += hw * 2) {
                if (occupied(pos.x + dx, pos.y + dy, newPos.z + (vel.z > 0 ? hw : -hw))) {
                    hit = true; goto zDone;
                }
            }
        }
        zDone:
        if (!hit) pos.z = newPos.z;
        else velocity_.z = 0;
    }

    if (std::abs(vel.y) > 0.001f) {
        bool hit = false;
        for (float dz = -hw; dz <= hw; dz += hw * 2) {
            for (float dx = -hw; dx <= hw; dx += hw * 2) {
                if (occupied(pos.x + dx, pos.y + (vel.y > 0 ? height_ : 0), pos.z + dz)) {
                    hit = true; goto yDone;
                }
            }
        }
        yDone:
        if (!hit) {
            pos.y = newPos.y;
            if (vel.y < 0) onGround_ = false;
        } else {
            if (vel.y < 0) onGround_ = true;
            velocity_.y = 0;
        }
    }

    return true;
}

void Player::update(float deltaTime, ChunkManager &chunkManager) {
    constexpr float gravity = -20.0f;
    constexpr float friction = 0.8f;

    Vector3 flatForward = camera_.getForward();
    flatForward.y = 0;
    if (flatForward.length() > 0) flatForward = flatForward.normalized();
    Vector3 flatRight = camera_.getRight();
    flatRight.y = 0;
    if (flatRight.length() > 0) flatRight = flatRight.normalized();

    Vector3 moveVec = flatForward * inputMove_.z + flatRight * inputMove_.x;
    if (moveVec.length() > 0) {
        moveVec = moveVec.normalized() * speed_;
    } else {
        moveVec.x = velocity_.x * friction;
        moveVec.z = velocity_.z * friction;
    }

    velocity_.x = moveVec.x;
    velocity_.z = moveVec.z;
    velocity_.y += gravity * deltaTime;
    if (velocity_.y < -30.0f) velocity_.y = -30.0f;

    if (position_.y < -10) {
        position_ = Vector3(0, 30, 0);
        velocity_ = Vector3(0, 0, 0);
        return;
    }

    collideAndSlide(position_, velocity_, deltaTime, chunkManager);

    inputMove_ = Vector3(0, 0, 0);
}
