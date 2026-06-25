#ifndef MORFEGA_PLAYER_H
#define MORFEGA_PLAYER_H

#include "Utility.h"
#include "Camera.h"
#include "ChunkManager.h"

class Player {
public:
    explicit Player(Camera &camera);

    void update(float deltaTime, ChunkManager &chunkManager);
    void move(const Vector3 &direction);
    void jump();

    Vector3 getPosition() const { return position_; }
    Vector3 getEyePosition() const { return position_ + Vector3(0, eyeHeight_, 0); }
    float getEyeHeight() const { return eyeHeight_; }

    bool isOnGround() const { return onGround_; }

private:
    bool collideAndSlide(Vector3 &pos, const Vector3 &vel, float deltaTime, ChunkManager &chunkManager);

    Camera &camera_;
    Vector3 position_;
    Vector3 velocity_;
    Vector3 inputMove_;

    float eyeHeight_ = 1.62f;
    float height_ = 1.8f;
    float width_ = 0.6f;
    float speed_ = 4.5f;
    float jumpVelocity_ = 8.0f;
    bool onGround_ = false;
};

#endif
