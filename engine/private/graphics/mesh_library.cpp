#include "graphics/mesh_library.h"

#include <cmath>

namespace golias {

    namespace {

        constexpr uint32_t kPosColorUVStride = 8; // 3 pos + 3 color + 2 uv

        Ref<Mesh> UploadMesh(const Ref<RHIDevice>& device, const std::vector<float>& vertices, const std::vector<uint32_t>& indices) {
            auto mesh         = std::make_shared<Mesh>();
            mesh->Vertices    = vertices;
            mesh->IndicesData = indices;
            mesh->VertexBuffers.push_back({0, kPosColorUVStride * sizeof(float), 0, false});
            mesh->VertexAttributes = {
                {0, 0, 0, VertexElementFormat::Float3},
                {1, 0, 12, VertexElementFormat::Float3},
                {2, 0, 24, VertexElementFormat::Float2},
            };
            mesh->VertexBuffer =
                device->CreateBuffer({.usage = BufferUsage::Vertex, .size = static_cast<uint32_t>(vertices.size() * sizeof(float))});

            mesh->IndexBuffer =
                device->CreateBuffer({.usage = BufferUsage::Index, .size = static_cast<uint32_t>(indices.size() * sizeof(uint32_t))});

            device->UploadToBuffer(mesh->VertexBuffer, vertices.data(), static_cast<uint32_t>(vertices.size() * sizeof(float)), 0);
            device->UploadToBuffer(mesh->IndexBuffer, indices.data(), static_cast<uint32_t>(indices.size() * sizeof(uint32_t)), 0);

            mesh->Submeshes.push_back({0, static_cast<uint32_t>(indices.size())});
            return mesh;
        }

        void PushVertex(std::vector<float>& v, float px, float py, float pz, float u, float vv) {
            v.push_back(px);
            v.push_back(py);
            v.push_back(pz);
            v.push_back(1.0f);
            v.push_back(1.0f);
            v.push_back(1.0f); // white color
            v.push_back(u);
            v.push_back(vv);
        }

        void PushQuad(std::vector<uint32_t>& idx, uint32_t base) {
            idx.push_back(base);
            idx.push_back(base + 1);
            idx.push_back(base + 2);
            idx.push_back(base + 2);
            idx.push_back(base + 3);
            idx.push_back(base);
        }

    } // namespace

    Ref<Mesh> MeshLibrary::CreateCube(const Ref<RHIDevice>& device) {
        std::vector<float> v;
        std::vector<uint32_t> idx;

        // Front face (z = +1)
        PushVertex(v, -1, -1, 1, 0, 1);
        PushVertex(v, 1, -1, 1, 1, 1);
        PushVertex(v, 1, 1, 1, 1, 0);
        PushVertex(v, -1, 1, 1, 0, 0);
        PushQuad(idx, 0);

        // Back face (z = -1)
        PushVertex(v, 1, -1, -1, 0, 1);
        PushVertex(v, -1, -1, -1, 1, 1);
        PushVertex(v, -1, 1, -1, 1, 0);
        PushVertex(v, 1, 1, -1, 0, 0);
        PushQuad(idx, 4);

        // Right face (x = +1)
        PushVertex(v, 1, -1, 1, 0, 1);
        PushVertex(v, 1, -1, -1, 1, 1);
        PushVertex(v, 1, 1, -1, 1, 0);
        PushVertex(v, 1, 1, 1, 0, 0);
        PushQuad(idx, 8);

        // Left face (x = -1)
        PushVertex(v, -1, -1, -1, 0, 1);
        PushVertex(v, -1, -1, 1, 1, 1);
        PushVertex(v, -1, 1, 1, 1, 0);
        PushVertex(v, -1, 1, -1, 0, 0);
        PushQuad(idx, 12);

        // Top face (y = +1)
        PushVertex(v, -1, 1, 1, 0, 1);
        PushVertex(v, 1, 1, 1, 1, 1);
        PushVertex(v, 1, 1, -1, 1, 0);
        PushVertex(v, -1, 1, -1, 0, 0);
        PushQuad(idx, 16);

        // Bottom face (y = -1)
        PushVertex(v, -1, -1, -1, 0, 1);
        PushVertex(v, 1, -1, -1, 1, 1);
        PushVertex(v, 1, -1, 1, 1, 0);
        PushVertex(v, -1, -1, 1, 0, 0);
        PushQuad(idx, 20);

        return UploadMesh(device, v, idx);
    }

    Ref<Mesh> MeshLibrary::CreatePlane(const Ref<RHIDevice>& device) {
        std::vector<float> v;
        std::vector<uint32_t> idx;

        // Plane on XZ, y=0, normals up
        PushVertex(v, -1, 0, -1, 0, 0);
        PushVertex(v, 1, 0, -1, 1, 0);
        PushVertex(v, 1, 0, 1, 1, 1);
        PushVertex(v, -1, 0, 1, 0, 1);
        PushQuad(idx, 0);

        return UploadMesh(device, v, idx);
    }

    Ref<Mesh> MeshLibrary::CreateQuad(const Ref<RHIDevice>& device) {
        std::vector<float> v;
        std::vector<uint32_t> idx;

        // Quad on XY, z=0
        PushVertex(v, -1, -1, 0, 0, 1);
        PushVertex(v, 1, -1, 0, 1, 1);
        PushVertex(v, 1, 1, 0, 1, 0);
        PushVertex(v, -1, 1, 0, 0, 0);
        PushQuad(idx, 0);

        return UploadMesh(device, v, idx);
    }

    Ref<Mesh> MeshLibrary::CreateSphere(const Ref<RHIDevice>& device, uint32_t segments, uint32_t rings) {
        std::vector<float> v;
        std::vector<uint32_t> idx;

        for (uint32_t ring = 0; ring <= rings; ++ring) {
            float phi    = static_cast<float>(ring) / static_cast<float>(rings) * glm::pi<float>();
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);

            for (uint32_t seg = 0; seg <= segments; ++seg) {
                float theta = static_cast<float>(seg) / static_cast<float>(segments) * 2.0f * glm::pi<float>();
                float x     = sinPhi * std::cos(theta);
                float y     = cosPhi;
                float z     = sinPhi * std::sin(theta);
                float u     = static_cast<float>(seg) / static_cast<float>(segments);
                float vv    = static_cast<float>(ring) / static_cast<float>(rings);
                PushVertex(v, x, y, z, u, vv);
            }
        }

        for (uint32_t ring = 0; ring < rings; ++ring) {
            for (uint32_t seg = 0; seg < segments; ++seg) {
                uint32_t a = ring * (segments + 1) + seg;
                uint32_t b = a + segments + 1;
                idx.push_back(a);
                idx.push_back(b);
                idx.push_back(a + 1);
                idx.push_back(a + 1);
                idx.push_back(b);
                idx.push_back(b + 1);
            }
        }

        return UploadMesh(device, v, idx);
    }

    Ref<Mesh> MeshLibrary::CreateCylinder(const Ref<RHIDevice>& device, uint32_t segments) {
        std::vector<float> v;
        std::vector<uint32_t> idx;

        float halfH = 1.0f;

        // Side vertices
        for (uint32_t i = 0; i <= segments; ++i) {
            float angle = static_cast<float>(i) / static_cast<float>(segments) * 2.0f * glm::pi<float>();
            float x     = std::cos(angle);
            float z     = std::sin(angle);
            float u     = static_cast<float>(i) / static_cast<float>(segments);

            PushVertex(v, x, -halfH, z, u, 1); // bottom ring
            PushVertex(v, x, halfH, z, u, 0); // top ring
        }

        // Side indices
        for (uint32_t i = 0; i < segments; ++i) {
            uint32_t base = i * 2;
            idx.push_back(base);
            idx.push_back(base + 1);
            idx.push_back(base + 2);
            idx.push_back(base + 2);
            idx.push_back(base + 1);
            idx.push_back(base + 3);
        }

        // Cap centers
        uint32_t bottomCenter = static_cast<uint32_t>(v.size() / kPosColorUVStride);
        PushVertex(v, 0, -halfH, 0, 0.5f, 0.5f);
        uint32_t topCenter = static_cast<uint32_t>(v.size() / kPosColorUVStride);
        PushVertex(v, 0, halfH, 0, 0.5f, 0.5f);

        // Bottom cap
        for (uint32_t i = 0; i < segments; ++i) {
            uint32_t a = i * 2;
            uint32_t b = ((i + 1) % (segments + 1)) * 2;
            idx.push_back(bottomCenter);
            idx.push_back(b);
            idx.push_back(a);
        }

        // Top cap
        for (uint32_t i = 0; i < segments; ++i) {
            uint32_t a = i * 2 + 1;
            uint32_t b = ((i + 1) % (segments + 1)) * 2 + 1;
            idx.push_back(topCenter);
            idx.push_back(a);
            idx.push_back(b);
        }

        return UploadMesh(device, v, idx);
    }

    Ref<Mesh> MeshLibrary::CreateCapsule(const Ref<RHIDevice>& device, uint32_t segments, uint32_t rings) {
        std::vector<float> v;
        std::vector<uint32_t> idx;

        constexpr float pi             = glm::pi<float>();
        const float bodyHalfHeight     = 1.0f;
        const float radius             = 0.5f;
        const uint32_t hemisphereRings = std::max(2u, rings / 2u);
        const uint32_t profileRings    = hemisphereRings * 2 + 1;

        for (uint32_t ring = 0; ring <= profileRings; ++ring) {
            float radial;
            float y;
            if (ring <= hemisphereRings) {
                const float angle = static_cast<float>(ring) / hemisphereRings * (pi * 0.5f);
                radial            = radius * std::sin(angle);
                y                 = bodyHalfHeight + radius * std::cos(angle);
            } else {
                const float angle = static_cast<float>(ring - hemisphereRings) / hemisphereRings * (pi * 0.5f);
                radial            = radius * std::cos(angle);
                y                 = -bodyHalfHeight - radius * std::sin(angle);
            }

            for (uint32_t seg = 0; seg <= segments; ++seg) {
                const float theta = static_cast<float>(seg) / segments * (2.0f * pi);
                PushVertex(v,
                           radial * std::cos(theta),
                           y,
                           radial * std::sin(theta),
                           static_cast<float>(seg) / segments,
                           static_cast<float>(ring) / profileRings);
            }
        }

        for (uint32_t ring = 0; ring < profileRings; ++ring) {
            for (uint32_t seg = 0; seg < segments; ++seg) {
                const uint32_t a = ring * (segments + 1) + seg;
                const uint32_t b = a + segments + 1;
                idx.push_back(a);
                idx.push_back(a + 1);
                idx.push_back(b);
                idx.push_back(a + 1);
                idx.push_back(b + 1);
                idx.push_back(b);
            }
        }

        return UploadMesh(device, v, idx);
    }

    Ref<Mesh> MeshLibrary::CreateTorus(const Ref<RHIDevice>& device, uint32_t segments, uint32_t rings) {
        std::vector<float> v;
        std::vector<uint32_t> idx;

        float majorR = 1.0f;
        float minorR = 0.35f;

        for (uint32_t ring = 0; ring <= rings; ++ring) {
            float theta = static_cast<float>(ring) / static_cast<float>(rings) * 2.0f * glm::pi<float>();
            float cosT  = std::cos(theta);
            float sinT  = std::sin(theta);

            for (uint32_t seg = 0; seg <= segments; ++seg) {
                float phi  = static_cast<float>(seg) / static_cast<float>(segments) * 2.0f * glm::pi<float>();
                float cosP = std::cos(phi);
                float sinP = std::sin(phi);

                float x  = (majorR + minorR * cosP) * cosT;
                float y  = minorR * sinP;
                float z  = (majorR + minorR * cosP) * sinT;
                float u  = static_cast<float>(ring) / static_cast<float>(rings);
                float vv = static_cast<float>(seg) / static_cast<float>(segments);
                PushVertex(v, x, y, z, u, vv);
            }
        }

        for (uint32_t ring = 0; ring < rings; ++ring) {
            for (uint32_t seg = 0; seg < segments; ++seg) {
                uint32_t a = ring * (segments + 1) + seg;
                uint32_t b = a + segments + 1;
                idx.push_back(a);
                idx.push_back(b);
                idx.push_back(a + 1);
                idx.push_back(a + 1);
                idx.push_back(b);
                idx.push_back(b + 1);
            }
        }

        return UploadMesh(device, v, idx);
    }

} // namespace golias
