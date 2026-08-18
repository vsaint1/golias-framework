#include "core/io/file_system.h"
#include "graphics/model_importer.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

namespace golias {
    namespace {
        void AddVertex(ImportedMesh& mesh, const glm::vec3& position, const glm::vec2& uv) {
            mesh.Vertices.insert(mesh.Vertices.end(), {position.x, position.y, position.z, 1, 1, 1, uv.x, uv.y});
        }

        class ObjImporter final : public ModelImporter {
        public:
            bool Import(const String& virtualPath, ImportedModel& output, String& error) const override {
                tinyobj::attrib_t attrib;
                std::vector<tinyobj::shape_t> shapes;
                std::vector<tinyobj::material_t> materials;
                std::string parsingError;
                const String physicalPath = FileSystem::GetInstance().Resolve(virtualPath);
                const String directory    = VFS::PhysicalDirectory(virtualPath);
                if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &parsingError, physicalPath.c_str(), directory.c_str(), true)) {
                    error = "Failed to parse OBJ '" + virtualPath + "': " + parsingError;
                    return false;
                }

                for (const auto& source : materials) {
                    ImportedMaterial material;
                    material.Name      = source.name;
                    material.BaseColor = {source.diffuse[0], source.diffuse[1], source.diffuse[2], source.dissolve};

                    if (!source.diffuse_texname.empty()) {
                        ImportedTexture texture;
                        texture.Name = source.diffuse_texname;
                        std::ifstream file(std::filesystem::path(directory) / source.diffuse_texname, std::ios::binary);
                        texture.Data.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
                        if (!texture.Data.empty()) {
                            material.BaseColorTexture = static_cast<int32_t>(output.Textures.size());
                            output.Textures.push_back(std::move(texture));
                        }
                    }

                    output.Materials.push_back(std::move(material));
                }

                for (const auto& shape : shapes) {
                    ImportedMesh mesh;
                    mesh.Name             = shape.name;
                    int32_t materialIndex = -1;

                    for (size_t face = 0; face < shape.mesh.num_face_vertices.size(); ++face) {
                        const size_t vertexCount = shape.mesh.num_face_vertices[face];
                        materialIndex            = shape.mesh.material_ids[face];
                        for (size_t vertex = 0; vertex < vertexCount; ++vertex) {
                            const tinyobj::index_t index = shape.mesh.indices[face * vertexCount + vertex];
                            const glm::vec3 position     = {attrib.vertices[3 * index.vertex_index],
                                                            attrib.vertices[3 * index.vertex_index + 1],
                                                            attrib.vertices[3 * index.vertex_index + 2]};
                            const glm::vec2 uv           = index.texcoord_index >= 0
                                                             ? glm::vec2(attrib.texcoords[2 * index.texcoord_index],
                                                                         1.0f - attrib.texcoords[2 * index.texcoord_index + 1])
                                                             : glm::vec2(0.0f);
                            AddVertex(mesh, position, uv);
                            mesh.Indices.push_back(static_cast<uint32_t>(mesh.Indices.size()));
                        }
                    }

                    mesh.MaterialIndex = materialIndex;
                    if (!mesh.Indices.empty()) {
                        output.Meshes.push_back(std::move(mesh));
                    }
                }

                ImportedNode root;
                root.Name = "Root";
                root.MeshIndices.resize(output.Meshes.size());
                std::iota(root.MeshIndices.begin(), root.MeshIndices.end(), 0);
                output.Nodes.push_back(std::move(root));
                return !output.Meshes.empty();
            }
        };
    } // namespace

    Ref<ModelImporter> CreateObjImporter() {
        return std::make_shared<ObjImporter>();
    }
} // namespace golias
