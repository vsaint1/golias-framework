#include "scene/scene.h"

#include "scene/components/3d/camera.h"
#include <algorithm>

namespace golias {

    Scene::Scene(const String& name) : mName(name) {
    }

    Scene::~Scene() {
        Clear();
    }

    const String& Scene::GetName() const {
        return mName;
    }

    void Scene::SetMainCamera(Camera* camera) {
        if (camera != nullptr && (camera->GetOwner() == nullptr || camera->GetOwner()->GetScene() != this)) {
            LOG_ERROR("Scene '{}' cannot adopt a camera that does not belong to this scene.", mName);
            return;
        }

        mMainCamera = camera;
    }

    Camera* Scene::GetMainCamera() const {
        return mMainCamera;
    }

    void Scene::RenameObject(GameObject* object, const String& newName) {
        if (object == nullptr || object->GetScene() != this) {
            return;
        }

        const String& oldName = object->mName;
        if (oldName == newName) {
            return;
        }

        RemoveFromNameIndex(object);
        object->mName = newName;
        mNameIndex[newName].push_back(object);
    }

    void Scene::RemoveObject(const String& name) {
        for (GameObject* object : FindObjectsByName<GameObject>(name)) {
            object->Destroy();
        }
    }

    void Scene::RemoveObject(GameObject* object) {
        if (object != nullptr) {
            object->Destroy();
        }
    }

    void Scene::Clear() {
        for (auto& object : mObjects) {
            object->OnDestroy();
        }
        mObjects.clear();
        mNameIndex.clear();
        mTypeIndex.clear();
        mMainCamera = nullptr;
    }

    const std::vector<std::unique_ptr<GameObject>>& Scene::GetObjects() const {
        return mObjects;
    }


    void Scene::Update(float deltaTime) {
    
        for (const auto& object : mObjects) {
            if (!object->mStarted) {
                object->Start();
                object->mStarted = true;
            }

            object->Update(deltaTime);
        }

        bool hasPendingDestroy = false;
        for (const auto& object : mObjects) {
            if (object->IsPendingDestroy()) {
                hasPendingDestroy = true;
                break;
            }
        }

        if (!hasPendingDestroy) {
            return;
        }

        std::vector<std::unique_ptr<GameObject>> survivors;
        survivors.reserve(mObjects.size());

        for (auto& object : mObjects) {
            if (object->IsPendingDestroy()) {
                // Fire OnDestroy, unregister from all indexes + fix
                // parent/child links, then let the unique_ptr destroy the
                // object as it leaves scope. All objects stay alive until
                // mObjects is reassigned below, so reparenting children to
                // soon-to-die parents stays valid.
                object->OnDestroy();
                UnregisterObject(object.get());
            } else {
                survivors.push_back(std::move(object));
            }
        }

        mObjects = std::move(survivors);
    }

    void Scene::UnregisterObject(GameObject* object) {
        RemoveFromNameIndex(object);
        RemoveFromTypeIndex(object);

        if (mMainCamera != nullptr && mMainCamera->GetOwner() == object) {
            mMainCamera = nullptr;
        }

        object->RemoveFromHierarchy();

        object->SetScene(nullptr);
    }

    void Scene::RemoveFromNameIndex(GameObject* object) {
        auto it = mNameIndex.find(object->mName);
        if (it != mNameIndex.end()) {
            std::erase(it->second, object);
            if (it->second.empty()) {
                mNameIndex.erase(it);
            }
        }
    }

    void Scene::RemoveFromTypeIndex(GameObject* object) {
        auto typeIt = mTypeIndex.find(std::type_index(typeid(*object)));
        if (typeIt != mTypeIndex.end()) {
            std::erase(typeIt->second, object);
            if (typeIt->second.empty()) {
                mTypeIndex.erase(typeIt);
            }
        }
    }

} // namespace golias
