#pragma once
#include "stdafx.h"

namespace golias {

    class Scene;
    class GameObject;


    class Component {
    public:
        Component() = default;
        virtual ~Component() = default;

        Component(const Component&)            = delete;
        Component& operator=(const Component&) = delete;

        GameObject* GetOwner() const;

        // Convenience: owner's Scene (nullptr while not in a scene).
        Scene* GetScene() const;

        virtual void OnAttached() {}

        virtual void Start() {}

        virtual void Update(float deltaTime) {}

        virtual void OnDetached() {}

        virtual void OnDestroy() {}

    private:
        friend class GameObject;
        void SetOwner(GameObject* owner);

        GameObject* mOwner           = nullptr;
        bool mStarted                = false;
        bool mPendingDestroy         = false;
    };

} // namespace golias
