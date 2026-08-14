#include "scene/components/3d/camera.h"

#include "scene/game_object.h"

#include <glm/gtc/matrix_transform.hpp>

namespace golias {

    Camera::Camera(float fov, float aspectRatio, float nearPlane, float farPlane)
        : mFov(fov), mAspect(aspectRatio), mNear(nearPlane), mFar(farPlane) {

        RecalculateProjection();
        RecalculateView();
    }

    void Camera::Update(float deltaTime) {
        // transform of the owning GameObject each frame.
        RecalculateView();
    }

    void Camera::LookAt(const glm::vec3& target) {
        GameObject* owner = GetOwner();
        if (owner == nullptr) {
            return;
        }

        // convention: left-handed world, +X right, +Y up, +Z forward.
        glm::mat4 view = glm::lookAtLH(owner->GetPosition(), target, glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 world = glm::inverse(view);
        glm::mat3 rotationBasis(world);

        owner->SetRotation(glm::quat_cast(rotationBasis));
        RecalculateView();
    }

    void Camera::SetFov(float fov) {
        mFov = fov;
        RecalculateProjection();
    }

    void Camera::SetAspectRatio(float aspectRatio) {
        mAspect = aspectRatio;
        RecalculateProjection();
    }

    float Camera::GetFov() const {
        return mFov;
    }

    float Camera::GetAspectRatio() const {
        return mAspect;
    }

    const glm::mat4& Camera::GetView() const {
        return mView;
    }

    const glm::mat4& Camera::GetProjection() const {
        return mProjection;
    }

    void Camera::RecalculateProjection() {
        mProjection = glm::perspectiveLH_ZO(glm::radians(mFov), mAspect, mNear, mFar);
    }

    void Camera::RecalculateView() {
        GameObject* owner = GetOwner();
        mView             = owner != nullptr ? glm::inverse(owner->GetWorldTransform()) : glm::mat4(1.0f);
    }

} // namespace golias
