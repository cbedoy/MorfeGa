#include "Camera.h"

Camera::Camera()
    : mode_(Mode::FIRST_PERSON)
    , position_(0, 0, 0)
    , pitch_(0)
    , yaw_(0)
    , thirdPersonDistance_(5.0f)
    , thirdPersonTargetDistance_(5.0f)
    , thirdPersonSmoothSpeed_(8.0f) {}

void Camera::rotate(float dx, float dy) {
    constexpr float kSensitivity = 0.003f;
    yaw_ -= dx * kSensitivity;
    pitch_ -= dy * kSensitivity;
    pitch_ = clamp(pitch_, -radians(89.0f), radians(89.0f));
}

void Camera::update(float deltaTime) {
    if (mode_ == Mode::THIRD_PERSON) {
        thirdPersonDistance_ += (thirdPersonTargetDistance_ - thirdPersonDistance_) * thirdPersonSmoothSpeed_ * deltaTime;
    }
}

Matrix4x4 Camera::getViewMatrix() const {
    if (mode_ == Mode::FIRST_PERSON) {
        Vector3 forward = getForward();
        Vector3 target = position_ + forward;
        return Matrix4x4::lookAt(position_, target, Vector3(0, 1, 0));
    } else {
        Vector3 forward = getForward();
        Vector3 eye = position_ - forward * thirdPersonDistance_;
        return Matrix4x4::lookAt(eye, position_, Vector3(0, 1, 0));
    }
}

Matrix4x4 Camera::getProjectionMatrix(float aspect) const {
    return Matrix4x4::perspective(radians(70.0f), aspect, 0.1f, 200.0f);
}

Matrix4x4 Camera::getViewProjectionMatrix(float aspect) const {
    return getProjectionMatrix(aspect) * getViewMatrix();
}

Vector3 Camera::getPosition() const {
    if (mode_ == Mode::FIRST_PERSON) {
        return position_;
    } else {
        Vector3 forward = getForward();
        return position_ - forward * thirdPersonDistance_;
    }
}

Vector3 Camera::getForward() const {
    return Vector3(
        std::sin(yaw_) * std::cos(pitch_),
        std::sin(pitch_),
        std::cos(yaw_) * std::cos(pitch_)
    ).normalized();
}

Vector3 Camera::getRight() const {
    return Vector3(std::cos(yaw_), 0, -std::sin(yaw_));
}

Vector3 Camera::getUp() const {
    Vector3 f = getForward();
    Vector3 r = getRight();
    return r.cross(f).normalized();
}

void Camera::setPosition(const Vector3 &pos) {
    position_ = pos;
}

void Camera::setTarget(const Vector3 &pos) {
    position_ = pos;
}

void Camera::setPitchYaw(float pitch, float yaw) {
    pitch_ = pitch;
    yaw_ = yaw;
}

void Camera::toggleMode() {
    mode_ = (mode_ == Mode::FIRST_PERSON) ? Mode::THIRD_PERSON : Mode::FIRST_PERSON;
}
