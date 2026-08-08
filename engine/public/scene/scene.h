#pragma once
#include "stdafx.h"
#include "scene/game_object.h"
#include "scene/components/common/tag.h"

#include <typeindex>
#include <type_traits>

namespace golias {

    class Camera;

    
    class Scene {
    public:
        explicit Scene(const String& name = "Scene");
        ~Scene();

        Scene(const Scene&)            = delete;
        Scene& operator=(const Scene&) = delete;

        const String& GetName() const;

     
        // Constructs T(name, args...) in place, registers it in the name +
        // tag + type indexes, sets its scene back-reference and returns a
        // non-owning T*. Names are not required to be unique (see INDEXING).
        template <typename T, typename... Args>
        T* AddObject(const String& name, Args&&... args) {
            static_assert(std::is_base_of_v<GameObject, T>, "AddObject<T> requires T to derive from GameObject.");

            auto object       = std::make_unique<T>(name, std::forward<Args>(args)...);
            T* raw            = object.get();
            GameObject* base  = static_cast<GameObject*>(raw);

            base->SetScene(this);
            mObjects.push_back(std::move(object));
            mNameIndex[name].push_back(base);
            mTypeIndex[std::type_index(typeid(T))].push_back(base);

            return raw;
        }

        // First object named `name` that is-a T (insertion order); nullptr when
        // none. Use FindObjectsByName for every match.
        template <typename T>
        T* FindObject(const String& name) const {
            auto it = mNameIndex.find(name);
            if (it == mNameIndex.end()) {
                return nullptr;
            }

            for (GameObject* object : it->second) {
                if (auto* casted = dynamic_cast<T*>(object)) {
                    return casted;
                }
            }

            return nullptr;
        }

        // Every object named `name` that is-a T.
        template <typename T>
        std::vector<T*> FindObjectsByName(const String& name) const {
            std::vector<T*> result;
            auto it = mNameIndex.find(name);
            if (it == mNameIndex.end()) {
                return result;
            }

            result.reserve(it->second.size());
            for (GameObject* object : it->second) {
                if (auto* casted = dynamic_cast<T*>(object)) {
                    result.push_back(casted);
                }
            }

            return result;
        }

        // First object whose Tag component equals `tag` and that is-a T;
        // nullptr when none. GameObjects without a Tag component never match.
        template <typename T>
        T* FindObjectWithTag(const String& tag) const {
            for (const auto& object : mObjects) {
                Tag* tagComponent = object->GetComponent<Tag>();
                if (tagComponent != nullptr && tagComponent->GetTag() == tag) {
                    if (auto* casted = dynamic_cast<T*>(object.get())) {
                        return casted;
                    }
                }
            }
            return nullptr;
        }

        // Every object whose Tag component equals `tag` and that is-a T.
        template <typename T>
        std::vector<T*> FindObjectsWithTag(const String& tag) const {
            std::vector<T*> result;
            for (const auto& object : mObjects) {
                Tag* tagComponent = object->GetComponent<Tag>();
                if (tagComponent != nullptr && tagComponent->GetTag() == tag) {
                    if (auto* casted = dynamic_cast<T*>(object.get())) {
                        result.push_back(casted);
                    }
                }
            }

            return result;
        }

        // Objects whose most-derived type is exactly T.
        template <typename T>
        std::vector<T*> FindObjectsOfType() const {
            std::vector<T*> result;

            auto it = mTypeIndex.find(std::type_index(typeid(T)));
            if (it != mTypeIndex.end()) {
                result.reserve(it->second.size());
                for (GameObject* object : it->second) {
                    result.push_back(static_cast<T*>(object));
                }

                return result;
            }

            for (const auto& object : mObjects) {
                if (auto* casted = dynamic_cast<T*>(object.get())) {
                    result.push_back(casted);
                }
            }

            return result;
        }

        // Every GameObject carrying a component of type T.
        template <typename T>
        std::vector<GameObject*> FindObjectsWithComponent() const {
            std::vector<GameObject*> result;
            for (const auto& object : mObjects) {
                if (object->GetComponent<T>() != nullptr) {
                    result.push_back(object.get());
                }
            }

            return result;
        }

        // The main camera is the one used for rendering the scene. It is not
        // required to be the only camera in the scene, but it is the one that is used for rendering.
        void SetMainCamera(Camera* camera);

        Camera* GetMainCamera() const;

        // Renames an already-registered object, keeping the name index in sync.
        void RenameObject(GameObject* object, const String& newName);

        // Deferred removal: marks the matching object(s) pending-destroy.
        void RemoveObject(const String& name);

        void RemoveObject(GameObject* object);

        // Destroys every object immediately and clears all indexes.
        void Clear();

        const std::vector<std::unique_ptr<GameObject>>& GetObjects() const;

        void Update(float deltaTime);

    private:
        void UnregisterObject(GameObject* object);

        void RemoveFromNameIndex(GameObject* object);

        void RemoveFromTypeIndex(GameObject* object);

    private:
        String mName;
        std::vector<std::unique_ptr<GameObject>> mObjects;
        std::unordered_map<String, std::vector<GameObject*>> mNameIndex;
        std::unordered_map<std::type_index, std::vector<GameObject*>> mTypeIndex;
        Camera* mMainCamera = nullptr;
    };

} // namespace golias
