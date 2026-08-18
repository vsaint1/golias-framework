#pragma once

#include "graphics/render_resources.h"

namespace golias {

    struct ImportedTexture {
        String Name;
        std::vector<uint8_t> Data;
        uint32_t Width = 0;
        uint32_t Height = 0;
        bool IsRawRGBA = false;
    };

    struct ImportedMaterial {
        String Name;
        glm::vec4 BaseColor = {1, 1, 1, 1};
        int32_t BaseColorTexture = -1;
    };

    struct ImportedMesh {
        String Name;
        // Position (3), vertex color (3), and UV (2)
        std::vector<float> Vertices;
        std::vector<uint32_t> Indices;
        int32_t MaterialIndex = -1;
    };

    struct ImportedNode {
        String Name;
        int32_t ParentIndex = -1;
        glm::vec3 Position = glm::vec3(0.0f);
        glm::quat Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 Scale = glm::vec3(1.0f);
        std::vector<uint32_t> MeshIndices;
    };

    struct ImportedModel {
        std::vector<ImportedMesh> Meshes;
        std::vector<ImportedNode> Nodes;
        std::vector<ImportedMaterial> Materials;
        std::vector<ImportedTexture> Textures;
    };

    class ModelImporter {
    public:
        virtual ~ModelImporter() = default;

        virtual bool Import(const String& virtualPath, ImportedModel& output, String& error) const = 0;

        static Ref<ModelImporter> ForPath(const String& virtualPath);
        static bool ImportFile(const String& virtualPath, ImportedModel& output, String& error);
    };

} // namespace golias
