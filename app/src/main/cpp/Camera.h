#ifndef MORFEGA_CAMERA_H
#define MORFEGA_CAMERA_H

#include "Utility.h"

class Camera {
public:
    enum class Mode { FIRST_PERSON, THIRD_PERSON };

    Camera();

    void update(float deltaTime);
    void rotate(float dx, float dy);

    Matrix4x4 getViewMatrix() const;
    Matrix4x4 getProjectionMatrix(float aspect) const;
    Matrix4x4 getViewProjectionMatrix(float aspect) const;

    Vector3 getPosition() const;
    Vector3 getForward() const;
    Vector3 getRight() const;
    Vector3 getUp() const;

    void setPosition(const Vector3 &pos);
    void setTarget(const Vector3 &pos);
    void setPitchYaw(float pitch, float yaw);

    void toggleMode();
    Mode getMode() const { return mode_; }
    bool isFirstPerson() const { return mode_ == Mode::FIRST_PERSON; }

    void setThirdPersonDistance(float d) { thirdPersonDistance_ = d; }
    float getThirdPersonDistance() const { return thirdPersonDistance_; }

    float getPitch() const { return pitch_; }
    float getYaw() const { return yaw_; }

private:
    Mode mode_;
    Vector3 position_;
    float pitch_;
    float yaw_;
    float thirdPersonDistance_;
    float thirdPersonTargetDistance_;
    float thirdPersonSmoothSpeed_;
};

#endif
