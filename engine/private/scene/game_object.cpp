#include "scene/game_object.h"

#include "scene/scene.h"
#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>

namespace golias {

    namespace {

        // Decomposes a TRS matrix back into position, rotation and scale. Used
        // when a re-parent must preserve the world transform.
        void DecomposeTransform(const glm::mat4& transform, glm::vec3& position, glm::quat& rotation, glm::vec3& scale) {
            position = glm::vec3(transform[3]);

            glm::mat3 basis(transform);
            scale = glm::vec3(glm::length(basis[0]), glm::length(basis[1]), glm::length(basis[2]));
            scale = glm::max(scale, glm::vec3(0.0001f));

            glm::mat3 rotationMatrix;
            rotationMatrix[0] = basis[0] / scale.x;
            rotationMatrix[1] = basis[1] / scale.y;
            rotationMatrix[2] = basis[2] / scale.z;

            rotation = glm::quat_cast(rotationMatrix);
        }

    } // namespace

    GameObject::GameObject(const String& name) : mName(name) {
    }

    const String& GameObject::GetName() const {
        return mName;
    }

    void GameObject::SetName(const String& name) {
        if (mScene != nullptr) {
            // Registered: route through the scene so the name index stays in sync.
            mScene->RenameObject(this, name);
        } else {
            mName = name;
        }
    }

    Scene* GameObject::GetScene() const {
        return mScene;
    }

    void GameObject::SetScene(Scene* scene) {
        mScene = scene;
    }

    void GameObject::Destroy() {
        mPendingDestroy = true;
    }

    bool GameObject::IsPendingDestroy() const {
        return mPendingDestroy;
    }

    void GameObject::Start() {
    }

    void GameObject::Update(float deltaTime) {
   
        for (auto& component : mComponents) {

            if (!component->mStarted) {
                component->Start();
                component->mStarted = true;
            }

            component->Update(deltaTime);
        }

        // destroy components marked for removal.
        SweepComponents();
    }

    void GameObject::OnDestroy() {
        for (auto& component : mComponents) {
            component->OnDetached();
            component->OnDestroy();
        }

        mComponents.clear();
    }

    void GameObject::RemoveComponent(Component* component) {
        if (component != nullptr && component->mOwner == this) {
            component->mPendingDestroy = true;
        }
    }

    void GameObject::SweepComponents() {
        if (mComponents.empty()) {
            return;
        }

        auto remove_predicate = [](std::unique_ptr<Component>& component) {
            if (component->mPendingDestroy) {
                component->OnDetached();
                component->OnDestroy();
                return true;
            }

            return false;
        };

        mComponents.erase(std::remove_if(mComponents.begin(), mComponents.end(), remove_predicate), mComponents.end());
    }

    GameObject* GameObject::GetParent() const {
        return mParent;
    }

    const std::vector<GameObject*>& GameObject::GetChildren() const {
        return mChildren;
    }

    bool GameObject::SetParent(GameObject* newParent, AttachmentRule rule) {
        if (newParent == this) {
            LOG_ERROR("GameObject '{}' cannot parent to itself.", mName);
            return false;
        }

        if (newParent != nullptr && newParent->mScene != mScene) {
            LOG_ERROR("GameObject '{}' cannot parent to '{}': objects must share the same Scene.", mName, newParent->mName);
            return false;
        }

        // Cycle check: walk up the proposed parent chain; if we are an ancestor
        // of it, parenting would form a cycle.
        for (GameObject* ancestor = newParent; ancestor != nullptr; ancestor = ancestor->mParent) {
            if (ancestor == this) {
                LOG_ERROR("GameObject '{}' cannot parent to a descendant '{}'.", mName, newParent->mName);
                return false;
            }
        }

        glm::mat4 worldTransform = GetWorldTransform();

        if (mParent != nullptr) {
            std::erase(mParent->mChildren, this);
        }

        mParent = newParent;

        if (mParent != nullptr) {
            mParent->mChildren.push_back(this);
        }

        // Attachment rule:
        //  - KeepRelative: the local transform stays untouched, the world
        //    transform changes to follow the new parent.
        //  - KeepWorld:    the world transform stays untouched, the local
        //    transform is recomputed so parentWorld * newLocal == worldTransform.
        if (rule == AttachmentRule::KeepWorld) {
            glm::mat4 parentWorld = (mParent != nullptr) ? mParent->GetWorldTransform() : glm::mat4(1.0f);
            glm::mat4 newLocal    = glm::inverse(parentWorld) * worldTransform;
            DecomposeTransform(newLocal, mPosition, mRotation, mScale);
        }

        return true;
    }

    void GameObject::RemoveFromHierarchy() {
        GameObject* formerParent = mParent;

        if (mParent != nullptr) {
            std::erase(mParent->mChildren, this);
            mParent = nullptr;
        }

        std::vector<GameObject*> children = mChildren;
        mChildren.clear();

        for (GameObject* child : children) {
            child->SetParent(formerParent, AttachmentRule::KeepWorld);
        }
    }

    void GameObject::SetPosition(const glm::vec3& position) {
        mPosition = position;
    }

    void GameObject::SetRotation(const glm::quat& rotation) {
        mRotation = rotation;
    }

    void GameObject::SetRotation(const glm::vec3& eulerDegrees) {
        mRotation = glm::quat(glm::radians(eulerDegrees));
    }

    void GameObject::SetScale(const glm::vec3& scale) {
        mScale = scale;
    }

    void GameObject::RotateLocal(const glm::vec3& axis, float degrees) {
        mRotation = glm::angleAxis(glm::radians(degrees), glm::normalize(axis)) * mRotation;
    }

    const glm::vec3& GameObject::GetPosition() const {
        return mPosition;
    }

    const glm::quat& GameObject::GetRotation() const {
        return mRotation;
    }

    const glm::vec3& GameObject::GetScale() const {
        return mScale;
    }

    glm::mat4 GameObject::GetLocalTransform() const {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), mPosition);
        transform           = transform * glm::mat4_cast(mRotation);
        transform           = transform * glm::scale(glm::mat4(1.0f), mScale);
        return transform;
    }

    glm::mat4 GameObject::GetWorldTransform() const {
        if (mParent == nullptr) {
            return GetLocalTransform();
        }

        return mParent->GetWorldTransform() * GetLocalTransform();
    }

} // namespace golias
