//
// Created by Gianni on 4/01/2025.
//

#include "camera.hpp"

static constexpr glm::mat4 sRightHandedBasis
{
    1.f, 0.f, 0.f, 0.f,
    0.f, -1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f
};

Camera::Camera(glm::vec3 position, float fovY, float width, float height, float nearZ, float farZ)
    : mView()
    , mBasis()
    , mPosition(position)
    , mTheta()
    , mPhi()
    , mProjection(glm::perspective(fovY, width / height, nearZ, farZ))
    , mFovY(fovY)
    , mAspectRatio(width / height)
    , mNearZ(nearZ)
    , mFarZ(farZ)
    , mPreviousMousePos()
    , mState(Camera::State::FIRST_PERSON)
    , mFlySpeed(5.f)
    , mPanSpeed(0.8f)
    , mZScrollOffset(1.f)
    , mRotateSensitivity(1.f)
    , mLeftButtonDown()
    , mRightButtonDown()
{
}

void Camera::setState(Camera::State state)
{
    mState = state;
}

void Camera::update(float dt)
{
    if (ImGui::IsWindowHovered())
    {
        mLeftButtonDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        mRightButtonDown = ImGui::IsMouseDown(ImGuiMouseButton_Right);
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        mLeftButtonDown = false;

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        mRightButtonDown = false;

    switch (mState)
    {
        case FIRST_PERSON:
            updateFirstPerson(dt);
            break;
        case VIEW_MODE:
            updateViewMode(dt);
            break;
        case EDITOR_MODE:
            updateEditorMode(dt);
            break;
    }

    calculateViewProjection();

    mPreviousMousePos.x = ImGui::GetMousePos().x;
    mPreviousMousePos.y = ImGui::GetMousePos().y;
}

const glm::mat4 &Camera::viewProjection() const
{
    return mViewProjection;
}

const glm::mat4 &Camera::view() const
{
    return mView;
}

const glm::mat4 &Camera::projection() const
{
    return mProjection;
}

const glm::vec3 &Camera::position() const
{
    return mPosition;
}

const glm::vec3 &Camera::front() const
{
    return mBasis[2];
}

const Camera::State Camera::state() const
{
    return mState;
}

float *Camera::fov()
{
    return &mFovY;
}

float *Camera::nearPlane()
{
    return &mNearZ;
}

float *Camera::farPlane()
{
    return &mFarZ;
}

float *Camera::flySpeed()
{
    return &mFlySpeed;
}

float *Camera::panSpeed()
{
    return &mPanSpeed;
}

float *Camera::zScrollOffset()
{
    return &mZScrollOffset;
}

float *Camera::rotateSensitivity()
{
    return &mRotateSensitivity;
}

void Camera::resize(uint32_t width, uint32_t height)
{
    mAspectRatio = static_cast<float>(glm::max(1u, width)) / static_cast<float>(glm::max(1u,height));
}

void Camera::scroll(float x, float y)
{
    switch (mState)
    {
        case FIRST_PERSON:
        {
            mFlySpeed += glm::sign(y);
            break;
        }
        case VIEW_MODE:
        case EDITOR_MODE:
        {
            mPosition += mBasis[2] * mZScrollOffset * glm::sign(y);
            break;
        }
    }
}

void Camera::updateFirstPerson(float dt)
{
    int z = 0;
    int x = 0;

    if (ImGui::IsWindowFocused())
    {
        if (ImGui::IsKeyDown(ImGuiKey_W))
            --z;
        if (ImGui::IsKeyDown(ImGuiKey_S))
            ++z;
        if (ImGui::IsKeyDown(ImGuiKey_A))
            --x;
        if (ImGui::IsKeyDown(ImGuiKey_D))
            ++x;
    }

    glm::vec2 mouseDt{};
    if (mLeftButtonDown)
    {
        mouseDt.x = ImGui::GetMousePos().x - mPreviousMousePos.x;
        mouseDt.y = ImGui::GetMousePos().y - mPreviousMousePos.y;
    }

    if (z || x || mouseDt.x || mouseDt.y)
    {
        float xOffset = mFlySpeed * dt * glm::sign(x);
        float zOffset = mFlySpeed * dt * glm::sign(z);
        mPosition += mBasis[0] * xOffset + mBasis[2] * zOffset;

        mTheta += mouseDt.x * dt * mRotateSensitivity;
        mPhi += -mouseDt.y * dt * mRotateSensitivity;
    }
}

void Camera::updateViewMode(float dt)
{
    glm::vec2 mouseDt;
    mouseDt.x = ImGui::GetMousePos().x - mPreviousMousePos.x;
    mouseDt.y = ImGui::GetMousePos().y - mPreviousMousePos.y;

    if (mouseDt.x || mouseDt.y)
    {
        if (mRightButtonDown)
        {
            mTheta += mouseDt.x * dt * mRotateSensitivity;
            mPhi += -mouseDt.y * dt * mRotateSensitivity;
        }
        else if (mLeftButtonDown)
        {
            float xOffset = mPanSpeed * dt * mouseDt.x;
            float yOffset = mPanSpeed * dt * mouseDt.y;
            mPosition += mBasis[0] * xOffset + mBasis[1] * -yOffset; // todo check
        }
    }
}

void Camera::updateEditorMode(float dt)
{
    glm::vec2 mouseDt{};
    if (mRightButtonDown)
    {
        mouseDt.x = ImGui::GetMousePos().x - mPreviousMousePos.x;
        mouseDt.y = ImGui::GetMousePos().y - mPreviousMousePos.y;
    }

    if (mouseDt.x || mouseDt.y)
    {
        mTheta += mouseDt.x * dt * mRotateSensitivity;
        mPhi += -mouseDt.y * dt * mRotateSensitivity;
    }
}

void Camera::calculateBasis()
{
    mBasis[2] = {
        glm::sin(mTheta) * glm::cos(mPhi),
        glm::sin(mPhi),
        glm::cos(mTheta) * glm::cos(mPhi)
    };

    mBasis[2] *= -1.f;
    mBasis[0] = glm::normalize(glm::cross(glm::vec3(0.f, 1.f, 0.f), mBasis[2]));
    mBasis[1] = glm::normalize(glm::cross(mBasis[0], -mBasis[2]));
}

void Camera::calculateViewProjection()
{
    calculateBasis();

    mView[0].x = mBasis[0].x;
    mView[0].y = mBasis[1].x;
    mView[0].z = mBasis[2].x;
    mView[1].x = mBasis[0].y;
    mView[1].y = mBasis[1].y;
    mView[1].z = mBasis[2].y;
    mView[2].x = mBasis[0].z;
    mView[2].y = mBasis[1].z;
    mView[2].z = mBasis[2].z;
    mView[3].x = -glm::dot(mPosition, mBasis[0]);
    mView[3].y = -glm::dot(mPosition, mBasis[1]);
    mView[3].z = -glm::dot(mPosition, mBasis[2]);
    mView[3].w = 1.f;

    mProjection = glm::perspective(mFovY, mAspectRatio, mNearZ, mFarZ);
    mViewProjection = mProjection * mView;
}