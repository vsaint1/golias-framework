#pragma once
#include "stdafx.h"

#include "scene/components/component.h"

#include <glm/glm.hpp>

namespace golias {


    class Camera : public Component {
    public:
        Camera(float fov = 60.0f,
               float aspectRatio = 16.0f / 9.0f,
               float nearPlane = 0.1f,
               float farPlane = 1000.0f);
        ~Camera() override = default;

        void Update(float deltaTime) override;

        // Orients the owning GameObject to look at `target` from its current world position.
        void LookAt(const glm::vec3& target);

        void SetFov(float fov);

        void SetAspectRatio(float aspectRatio);

        float GetFov() const;

        float GetAspectRatio() const;

        const glm::mat4& GetView() const;

        const glm::mat4& GetProjection() const;

    private:
        void RecalculateView();
        void RecalculateProjection();

    private:
        float mFov  = 60.0f;
        float mAspect = 16.0f / 9.0f;
        float mNear = 0.1f;
        float mFar  = 1000.0f;

        glm::mat4 mView       = glm::mat4(1.0f);
        glm::mat4 mProjection = glm::mat4(1.0f);
    };

} // namespace golias
