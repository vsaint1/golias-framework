#include "core/io/file.h"
#include "core/io/file_system.h"
#include "graphics/model_importer.h"

#define TINYGLTF_IMPLEMENTATION
#include "tiny_gltf.h"


namespace golias {
    namespace {
        void AddVertex(ImportedMesh& mesh, const glm::vec3& position, const glm::vec3& color, const glm::vec2& uv) {
            mesh.Vertices.insert(mesh.Vertices.end(), {position.x, position.y, position.z, color.r, color.g, color.b, uv.x, uv.y});
        }

        class GltfImporter final : public ModelImporter {
        public:
            bool Import(const String& virtualPath, ImportedModel& output, String& error) const override {
                const auto data = VFS::ReadBinary(virtualPath);
                if (!data || data->empty()) {
                    error = "Failed to read glTF file '" + virtualPath + "'.";
                    return false;
                }

                tinygltf::TinyGLTF loader;
                tinygltf::Model model;
                std::string warning;
                std::string parsingError;
                const String physicalDirectory = VFS::PhysicalDirectory(virtualPath);
                bool loaded                    = false;
                if (file::Extension(virtualPath) == ".glb") {
                    loaded = loader.LoadBinaryFromMemory(
                        &model, &parsingError, &warning, data->data(), static_cast<unsigned int>(data->size()), physicalDirectory);
                } else {
                    const std::string source(data->begin(), data->end());
                    loaded = loader.LoadASCIIFromString(
                        &model, &parsingError, &warning, source.c_str(), static_cast<unsigned int>(source.size()), physicalDirectory);
                }

                if (!warning.empty()) {
                    LOG_WARN("glTF warning '{}': {}", virtualPath, warning);
                }

                if (!loaded) {
                    error = "Failed to parse glTF '" + virtualPath + "': " + parsingError;
                    return false;
                }

                std::vector<int32_t> imageToTexture(model.images.size(), -1);
                for (size_t i = 0; i < model.images.size(); ++i) {
                    const auto& image = model.images[i];
                    if (image.image.empty() || image.width <= 0 || image.height <= 0) {
                        continue;
                    }

                    ImportedTexture texture;
                    texture.Name      = image.uri.empty() ? "embedded-image-" + std::to_string(i) : image.uri;
                    texture.Width     = image.width;
                    texture.Height    = image.height;
                    texture.IsRawRGBA = true;
                    texture.Data.resize(static_cast<size_t>(image.width) * static_cast<size_t>(image.height) * 4);

                    for (size_t pixel = 0; pixel < static_cast<size_t>(image.width) * image.height; ++pixel) {
                        const size_t source         = pixel * static_cast<size_t>(image.component);
                        texture.Data[pixel * 4 + 0] = image.image[source + 0];
                        texture.Data[pixel * 4 + 1] = image.component > 1 ? image.image[source + 1] : image.image[source];
                        texture.Data[pixel * 4 + 2] = image.component > 2 ? image.image[source + 2] : image.image[source];
                        texture.Data[pixel * 4 + 3] = image.component > 3 ? image.image[source + 3] : 255;
                    }

                    imageToTexture[i] = static_cast<int32_t>(output.Textures.size());
                    output.Textures.push_back(std::move(texture));
                }

                for (const auto& material : model.materials) {
                    ImportedMaterial result;
                    result.Name = material.name;
                    if (material.pbrMetallicRoughness.baseColorFactor.size() == 4) {
                        result.BaseColor = {static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[0]),
                                            static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[1]),
                                            static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[2]),
                                            static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[3])};
                    }

                    const int textureIndex = material.pbrMetallicRoughness.baseColorTexture.index;
                    if (textureIndex >= 0 && textureIndex < static_cast<int>(model.textures.size())) {
                        const int imageIndex = model.textures[textureIndex].source;
                        if (imageIndex >= 0 && imageIndex < static_cast<int>(imageToTexture.size())) {
                            result.BaseColorTexture = imageToTexture[imageIndex];
                        }
                    }

                    output.Materials.push_back(std::move(result));
                }

                std::vector<std::vector<uint32_t>> meshToParts(model.meshes.size());
                for (size_t sourceMeshIndex = 0; sourceMeshIndex < model.meshes.size(); ++sourceMeshIndex) {
                    const auto& sourceMesh = model.meshes[sourceMeshIndex];
                    for (const auto& primitive : sourceMesh.primitives) {
                        if (primitive.mode != TINYGLTF_MODE_TRIANGLES) {
                            continue;
                        }

                        const auto positionIt = primitive.attributes.find("POSITION");
                        if (positionIt == primitive.attributes.end()) {
                            continue;
                        }

                        ImportedMesh mesh;
                        mesh.Name             = sourceMesh.name;
                        mesh.MaterialIndex    = primitive.material;
                        const auto& positions = model.accessors[positionIt->second];
                        const auto* colors    = FindAccessor(model, primitive, "COLOR_0");
                        const auto* uvs       = FindAccessor(model, primitive, "TEXCOORD_0");

                        for (uint32_t index : ReadIndices(model, primitive, positions.count)) {
                            AddVertex(mesh,
                                      ReadVec3(model, positions, index),
                                      colors ? ReadVec3(model, *colors, index) : glm::vec3(1.0f),
                                      uvs ? ReadVec2(model, *uvs, index) : glm::vec2(0.0f));
                            mesh.Indices.push_back(static_cast<uint32_t>(mesh.Indices.size()));
                        }

                        if (!mesh.Indices.empty()) {
                            const uint32_t partIndex = static_cast<uint32_t>(output.Meshes.size());
                            output.Meshes.push_back(std::move(mesh));
                            meshToParts[sourceMeshIndex].push_back(partIndex);
                        }
                    }
                }

                output.Nodes.resize(model.nodes.size());
                for (size_t nodeIndex = 0; nodeIndex < model.nodes.size(); ++nodeIndex) {
                    const auto& sourceNode = model.nodes[nodeIndex];
                    ImportedNode& node     = output.Nodes[nodeIndex];
                    node.Name              = sourceNode.name.empty() ? "Node-" + std::to_string(nodeIndex) : sourceNode.name;
                    for (int child : sourceNode.children) {
                        if (child >= 0 && child < static_cast<int>(output.Nodes.size())) {
                            output.Nodes[child].ParentIndex = static_cast<int32_t>(nodeIndex);
                        }
                    }

                    if (sourceNode.mesh >= 0 && sourceNode.mesh < static_cast<int>(meshToParts.size())) {
                        node.MeshIndices = meshToParts[sourceNode.mesh];
                    }

                    if (sourceNode.matrix.size() == 16) {
                        glm::mat4 matrix(1.0f);
                        for (int i = 0; i < 16; ++i) {
                            matrix[i % 4][i / 4] = static_cast<float>(sourceNode.matrix[i]);
                        }
                        
                        glm::vec3 skew;
                        glm::vec4 perspective;
                        glm::decompose(matrix, node.Scale, node.Rotation, node.Position, skew, perspective);
                    } else {
                        if (sourceNode.translation.size() == 3) {
                            node.Position = {static_cast<float>(sourceNode.translation[0]),
                                             static_cast<float>(sourceNode.translation[1]),
                                             static_cast<float>(sourceNode.translation[2])};
                        }

                        if (sourceNode.rotation.size() == 4) {
                            node.Rotation = {static_cast<float>(sourceNode.rotation[3]),
                                             static_cast<float>(sourceNode.rotation[0]),
                                             static_cast<float>(sourceNode.rotation[1]),
                                             static_cast<float>(sourceNode.rotation[2])};
                        }

                        if (sourceNode.scale.size() == 3) {
                            node.Scale = {static_cast<float>(sourceNode.scale[0]),
                                          static_cast<float>(sourceNode.scale[1]),
                                          static_cast<float>(sourceNode.scale[2])};
                        }
                    }
                }

                if (output.Nodes.empty()) {
                    ImportedNode root;
                    root.Name = "Root";
                    root.MeshIndices.resize(output.Meshes.size());
                    std::iota(root.MeshIndices.begin(), root.MeshIndices.end(), 0);
                    output.Nodes.push_back(std::move(root));
                }

                return !output.Meshes.empty();
            }

        private:
            static const tinygltf::Accessor*
                FindAccessor(const tinygltf::Model& model, const tinygltf::Primitive& primitive, const char* name) {
                const auto it = primitive.attributes.find(name);
                return it == primitive.attributes.end() ? nullptr : &model.accessors[it->second];
            }

            static size_t ComponentSize(int componentType) {
                return componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE || componentType == TINYGLTF_COMPONENT_TYPE_BYTE   ? 1
                     : componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT || componentType == TINYGLTF_COMPONENT_TYPE_SHORT ? 2
                                                                                                                                 : 4;
            }

            static size_t ComponentCount(int type) {
                return type == TINYGLTF_TYPE_SCALAR ? 1 : type == TINYGLTF_TYPE_VEC2 ? 2 : type == TINYGLTF_TYPE_VEC3 ? 3 : 4;
            }

            static const unsigned char* Data(const tinygltf::Model& model, const tinygltf::Accessor& accessor, size_t index) {
                const auto& view    = model.bufferViews[accessor.bufferView];
                const auto& buffer  = model.buffers[view.buffer];
                const size_t stride = accessor.ByteStride(view) > 0 ? accessor.ByteStride(view)
                                                                    : ComponentSize(accessor.componentType) * ComponentCount(accessor.type);
                return buffer.data.data() + view.byteOffset + accessor.byteOffset + index * stride;
            }

            static float Component(const unsigned char* data, int type, bool normalized) {
                float value = 0.0f;
                if (type == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                    value = *reinterpret_cast<const float*>(data);
                } else if (type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                    value = normalized ? *data / 255.0f : *data;
                } else if (type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                    value = normalized ? *reinterpret_cast<const uint16_t*>(data) / 65535.0f : *reinterpret_cast<const uint16_t*>(data);
                }

                return value;
            }

            static glm::vec2 ReadVec2(const tinygltf::Model& model, const tinygltf::Accessor& accessor, size_t index) {
                const auto* data  = Data(model, accessor, index);
                const size_t size = ComponentSize(accessor.componentType);
                return {Component(data, accessor.componentType, accessor.normalized),
                        Component(data + size, accessor.componentType, accessor.normalized)};
            }

            static glm::vec3 ReadVec3(const tinygltf::Model& model, const tinygltf::Accessor& accessor, size_t index) {
                const auto* data  = Data(model, accessor, index);
                const size_t size = ComponentSize(accessor.componentType);
                return {Component(data, accessor.componentType, accessor.normalized),
                        Component(data + size, accessor.componentType, accessor.normalized),
                        Component(data + size * 2, accessor.componentType, accessor.normalized)};
            }

            static std::vector<uint32_t> ReadIndices(const tinygltf::Model& model, const tinygltf::Primitive& primitive, size_t count) {
                if (primitive.indices < 0) {
                    std::vector<uint32_t> result(count);
                    std::iota(result.begin(), result.end(), 0);
                    return result;
                }

                const auto& accessor = model.accessors[primitive.indices];
                std::vector<uint32_t> result;
                result.reserve(accessor.count);
                for (size_t i = 0; i < accessor.count; ++i) {
                    const auto* data = Data(model, accessor, i);
                    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                        result.push_back(*data);
                    } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                        result.push_back(*reinterpret_cast<const uint16_t*>(data));
                    } else {
                        result.push_back(*reinterpret_cast<const uint32_t*>(data));
                    }
                }

                return result;
            }
        };
    } // namespace

    Ref<ModelImporter> CreateGltfImporter() {
        return std::make_shared<GltfImporter>();
    }
} // namespace golias
