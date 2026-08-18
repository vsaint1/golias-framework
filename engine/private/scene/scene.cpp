#include "scene/scene.h"

#include "graphics/render_resources.h"
#include "scene/components/3d/camera.h"
#include "scene/components/3d/mesh_filter.h"
#include "scene/components/3d/mesh_renderer.h"

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

    GameObject* Scene::Instantiate(const Ref<Model>& model, const String& name) {
        return InstantiateModel(model, name);
    }

    GameObject* Scene::InstantiateModel(const Ref<Model>& model, const String& name) {
        if (!model || model->Nodes.empty()) {
            LOG_ERROR("Scene '{}' cannot instantiate an empty model.", mName);
            return nullptr;
        }

        GameObject* root = AddObject<GameObject>(name);
        std::vector<GameObject*> objects(model->Nodes.size(), nullptr);

        auto attach_part = [&](GameObject* object, uint32_t partIndex, const String& partName) {
            if (partIndex >= model->Parts.size()) {
                return;
            }

            const ModelPart& part = model->Parts[partIndex];
            if (!part.Mesh || !part.Material) {
                return;
            }

            GameObject* renderObject = object;
            if (object->GetComponent<MeshFilter>() != nullptr) {
                renderObject = AddObject<GameObject>(partName);
                renderObject->SetParent(object);
            }

            renderObject->AddComponent<MeshFilter>()->SetMesh(part.Mesh);

            auto material    = std::make_shared<MaterialInstance>();
            material->Parent = part.Material;
            
            renderObject->AddComponent<MeshRenderer>()->SetMaterial(material);
        };

        std::function<void(uint32_t, GameObject*)> instantiate_node;

        instantiate_node = [&](uint32_t nodeIndex, GameObject* parent) {
            if (nodeIndex >= model->Nodes.size() || objects[nodeIndex] != nullptr) {
                return;
            }

            const ModelNode& node = model->Nodes[nodeIndex];
            GameObject* object    = AddObject<GameObject>(node.Name);
            object->SetPosition(node.Position);
            object->SetRotation(node.Rotation);
            object->SetScale(node.Scale);
            object->SetParent(parent);

            objects[nodeIndex] = object;

            for (uint32_t partIndex : node.PartIndices) {
                attach_part(object, partIndex, node.Name + ".Mesh");
            }

            for (uint32_t childIndex = 0; childIndex < model->Nodes.size(); ++childIndex) {
                if (model->Nodes[childIndex].ParentIndex == static_cast<int32_t>(nodeIndex)) {
                    instantiate_node(childIndex, object);
                }
            }
        };

        for (uint32_t nodeIndex = 0; nodeIndex < model->Nodes.size(); ++nodeIndex) {
            if (model->Nodes[nodeIndex].ParentIndex < 0) {
                instantiate_node(nodeIndex, root);
            }
        }

        return root;
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

    void Scene::PrintTree() const {
        String tree = "Scene '" + mName + "'\n";

        std::function<void(const GameObject*, const String&, bool)> append_object;

        append_object = [&](const GameObject* object, const String& prefix, bool last) {
            tree += prefix + (last ? "`-- " : "|-- ") + object->GetName() + "\n";

            const auto& children     = object->GetChildren();
            const String childPrefix = prefix + (last ? "    " : "|   ");
            for (size_t i = 0; i < children.size(); ++i) {
                append_object(children[i], childPrefix, i + 1 == children.size());
            }
        };

        std::vector<GameObject*> roots;
        roots.reserve(mObjects.size());
        for (const auto& object : mObjects) {
            if (object->GetParent() == nullptr) {
                roots.push_back(object.get());
            }
        }

        for (size_t i = 0; i < roots.size(); ++i) {
            append_object(roots[i], "", i + 1 == roots.size());
        }

        LOG_INFO("{}", tree);
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
