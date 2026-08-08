#pragma once
#include "scene/components/component.h"
#include "stdafx.h"
#include <type_traits>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace golias {

    class Scene;

    // Attachment rule when re-parenting an object.
    enum class AttachmentRule {
        KeepRelative, // Preserve the object's local transform; the world may change.
        KeepWorld, // Preserve the object's world transform; local is recomputed.
    };

    class GameObject {
    public:
        explicit GameObject(const String& name = "GameObject");
        virtual ~GameObject() = default;

        GameObject(const GameObject&)            = delete;
        GameObject& operator=(const GameObject&) = delete;

        // Identity
        const String& GetName() const;

        // Rename the object.
        void SetName(const String& name);

        Scene* GetScene() const;

        // Marks the object for destruction. Removal is deferred to the end of the next Scene update.
        void Destroy();

        bool IsPendingDestroy() const;

        // Called exactly once on the first scene update after this object is
        // added, before the first Update(). Objects that are added and
        // destroyed within the same frame never receive it.
        virtual void Start();

        // Per-frame tick. May call Destroy(). The default
        // forwards to every component's Start/Update and sweeps destroyed
        // components; overrides should call GameObject::Update() to keep
        // components ticking.
        virtual void Update(float deltaTime);

        // Called exactly once when the object is destroyed: when a pending
        // Destroy() is swept at the end of a scene update, or during scene
        // teardown. Unregistered and freed right after.
        virtual void OnDestroy();

        // Constructs a component of type T in place, attaches it to this
        // object (owner = this, then T::OnAttached()) and returns a non-owning
        // T*.
        template <typename T, typename... Args>
        T* AddComponent(Args&&... args) {
            static_assert(std::is_base_of_v<Component, T>, "AddComponent<T> requires T to derive from Component.");

            auto component = std::make_unique<T>(std::forward<Args>(args)...);
            T* raw         = component.get();
            raw->SetOwner(this);
            mComponents.push_back(std::move(component));
            raw->OnAttached();
            return raw;
        }

        // First component of type T (in component-insertion order); nullptr
        // when the object has none. GetComponents<T> returns every match.
        template <typename T>
        T* GetComponent() const {
            for (const auto& component : mComponents) {
                if (auto* casted = dynamic_cast<T*>(component.get())) {
                    return casted;
                }
            }
            
            return nullptr;
        }

        template <typename T>
        std::vector<T*> GetComponents() const {
            std::vector<T*> result;
            for (const auto& component : mComponents) {
                if (auto* casted = dynamic_cast<T*>(component.get())) {
                    result.push_back(casted);
                }
            }

            return result;
        }

        // Marks a component for removal; it is destroyed (OnDestroy) and
        // removed at the end of this object's next Update() sweep.
        void RemoveComponent(Component* component);

        // Marks every component of type T for removal.
        template <typename T>
        void RemoveComponents() {
            for (const auto& component : mComponents) {
                if (dynamic_cast<T*>(component.get()) != nullptr) {
                    RemoveComponent(component.get());
                }
            }
        }

        // Children are NOT owned by the parent; the Scene owns every object.
        // These are raw, non-owning links so scene-wide lookup and the
        // deferred-destroy sweep keep working uniformly.
        GameObject* GetParent() const;

        // Re-parents this object. Returns false (and changes nothing).
        bool SetParent(GameObject* parent, AttachmentRule rule = AttachmentRule::KeepRelative);

        const std::vector<GameObject*>& GetChildren() const;

        //  Transform (local vs world)
        void SetPosition(const glm::vec3& position);

        void SetRotation(const glm::quat& rotation);

        void SetRotation(const glm::vec3& eulerDegrees);

        void SetScale(const glm::vec3& scale);

        // Composes an additional local rotation around `axis` by `degrees`.
        void RotateLocal(const glm::vec3& axis, float degrees);

        const glm::vec3& GetPosition() const;

        const glm::quat& GetRotation() const;

        const glm::vec3& GetScale() const;

        // Local space: T * R * S relative to the parent.
        glm::mat4 GetLocalTransform() const;

        // World space: parent's world transform * local (identity when root).
        glm::mat4 GetWorldTransform() const;

    private:
        void SetScene(Scene* scene);

        // Detaches this object from its parent and re-parents its own children
        // to the former parent (preserving their world transforms).
        void RemoveFromHierarchy();

        // Fires OnDestroy on and drops every component pending removal.
        void SweepComponents();

        friend class Scene;

    private:
        String mName;
        Scene* mScene        = nullptr;
        bool mPendingDestroy = false;
        bool mStarted        = false;

        std::vector<std::unique_ptr<Component>> mComponents;

        GameObject* mParent = nullptr;
        std::vector<GameObject*> mChildren;

        glm::vec3 mPosition = glm::vec3(0.0f);
        glm::quat mRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 mScale    = glm::vec3(1.0f);
    };

} // namespace golias
