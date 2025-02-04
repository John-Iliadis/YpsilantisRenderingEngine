//
// Created by Gianni on 4/01/2025.
//

#ifndef VULKANRENDERINGENGINE_CAMERA_HPP
#define VULKANRENDERINGENGINE_CAMERA_HPP

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "imgui/imgui.h"
#include "../utils/utils.hpp"

// todo: add key shortcuts for changing state
class Camera
{
public:
    enum State
    {
        FIRST_PERSON,
        VIEW_MODE,
        EDITOR_MODE
    };

public:
    Camera() = default;
    Camera(glm::vec3 position, float fovY, float width, float height, float nearZ = 0.1f, float farZ = 100.f);

    void setState(State state);
    void resize(uint32_t width, uint32_t height);
    void scroll(float x, float y);
    void update(float dt);

    const glm::mat4& viewProjection() const;
    const glm::mat4& view() const;
    const glm::mat4& projection() const;
    const glm::vec3& position() const;
    const glm::vec3& front() const;
    const State state() const;

    float* fov();
    float* nearPlane();
    float* farPlane();
    float* flySpeed();
    float* panSpeed();
    float* zScrollOffset();
    float* rotateSensitivity();

private:
    void updateFirstPerson(float dt);
    void updateViewMode(float dt);
    void updateEditorMode(float dt);
    void calculateBasis();
    void calculateViewProjection();

private:
    glm::mat4 mView;
    glm::mat3 mBasis;
    glm::vec3 mPosition;
    float mTheta;
    float mPhi;

    glm::mat4 mProjection;
    float mFovY;
    float mAspectRatio;
    float mNearZ;
    float mFarZ;

    glm::mat4 mViewProjection;

    glm::vec2 mPreviousMousePos;
    State mState;
    float mFlySpeed;
    float mPanSpeed;
    float mZScrollOffset;
    float mRotateSensitivity;
    bool mLeftButtonDown;
    bool mRightButtonDown;
};

#endif //VULKANRENDERINGENGINE_CAMERA_HPP
