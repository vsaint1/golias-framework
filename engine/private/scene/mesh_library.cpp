#include "scene/mesh_library.h"

#include <glm/gtc/constants.hpp>

namespace golias {

    namespace internal {

        MeshData CreateCube(float size) {
            const float h = size * 0.5f;

            MeshData mesh;
            mesh.Vertices = {
                // -X side (red)
                {{-h, -h, -h}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
                {{-h, -h, h},  {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
                {{-h, h, h},   {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
                {{-h, h, -h},  {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
                // +X side (green)
                {{h, -h, h},   {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
                {{h, -h, -h},  {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
                {{h, h, -h},   {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
                {{h, h, h},    {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
                // -Z side (blue)
                {{-h, -h, h},  {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
                {{h, -h, h},   {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
                {{h, h, h},    {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                {{-h, h, h},   {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
                // +Z side (yellow)
                {{h, -h, -h},  {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
                {{-h, -h, -h}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
                {{-h, h, -h},  {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
                {{h, h, -h},   {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
                // -Y side (cyan)
                {{-h, h, -h},  {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
                {{-h, h, h},   {0.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
                {{h, h, h},    {0.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
                {{h, h, -h},   {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
                // +Y side (magenta)
                {{h, -h, h},   {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
                {{-h, -h, h},  {1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
                {{-h, -h, -h}, {1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                {{h, -h, -h},  {1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
            };

            const uint32_t cubeIndices[] = {
                0,  1,  2,  0,  2,  3,  4,  5,  6,  4,  6,  7,  8,  9,  10, 8,  10, 11,
                12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23,
            };

            for (uint32_t index : cubeIndices) {
                mesh.Indices.push_back(index);
            }

            return mesh;
        }

        MeshData CreatePlane(float width, float depth, uint32_t segmentsX, uint32_t segmentsZ) {
            MeshData mesh;

            for (uint32_t z = 0; z <= segmentsZ; ++z) {
                const float vz = static_cast<float>(z) / static_cast<float>(segmentsZ);
                for (uint32_t x = 0; x <= segmentsX; ++x) {
                    const float vx = static_cast<float>(x) / static_cast<float>(segmentsX);

                    Vertex vertex;
                    vertex.Position = glm::vec3((vx - 0.5f) * width, 0.0f, (vz - 0.5f) * depth);
                    vertex.Color    = glm::vec3(vx, 0.5f, vz);
                    vertex.UV       = glm::vec2(vx, vz);
                    mesh.Vertices.push_back(vertex);
                }
            }

            for (uint32_t z = 0; z < segmentsZ; ++z) {
                for (uint32_t x = 0; x < segmentsX; ++x) {
                    const uint32_t topLeft     = z * (segmentsX + 1) + x;
                    const uint32_t topRight    = topLeft + 1;
                    const uint32_t bottomLeft  = (z + 1) * (segmentsX + 1) + x;
                    const uint32_t bottomRight = bottomLeft + 1;

                    mesh.Indices.push_back(topLeft);
                    mesh.Indices.push_back(bottomLeft);
                    mesh.Indices.push_back(topRight);

                    mesh.Indices.push_back(topRight);
                    mesh.Indices.push_back(bottomLeft);
                    mesh.Indices.push_back(bottomRight);
                }
            }

            return mesh;
        }

        MeshData CreateSphere(float radius, uint32_t segments, uint32_t rings) {
            MeshData mesh;

            // Vertices: (rings + 1) rows of (segments + 1) columns.
            for (uint32_t r = 0; r <= rings; ++r) {
                const float phi    = glm::pi<float>() * static_cast<float>(r) / static_cast<float>(rings);
                const float sinPhi = glm::sin(phi);
                const float cosPhi = glm::cos(phi);

                for (uint32_t s = 0; s <= segments; ++s) {
                    const float theta    = glm::two_pi<float>() * static_cast<float>(s) / static_cast<float>(segments);
                    const float sinTheta = glm::sin(theta);
                    const float cosTheta = glm::cos(theta);

                    Vertex vertex;
                    vertex.Position = glm::vec3(sinPhi * cosTheta, cosPhi, sinPhi * sinTheta) * radius;
                    // Deterministic color derived from the surface normal.
                    vertex.Color = vertex.Position / radius * 0.5f + 0.5f;
                    vertex.UV =
                        glm::vec2(static_cast<float>(s) / static_cast<float>(segments), static_cast<float>(r) / static_cast<float>(rings));
                    mesh.Vertices.push_back(vertex);
                }
            }

            for (uint32_t r = 0; r < rings; ++r) {
                for (uint32_t s = 0; s < segments; ++s) {
                    const uint32_t topLeft     = r * (segments + 1) + s;
                    const uint32_t topRight    = topLeft + 1;
                    const uint32_t bottomLeft  = (r + 1) * (segments + 1) + s;
                    const uint32_t bottomRight = bottomLeft + 1;

                    mesh.Indices.push_back(topLeft);
                    mesh.Indices.push_back(bottomLeft);
                    mesh.Indices.push_back(topRight);

                    mesh.Indices.push_back(topRight);
                    mesh.Indices.push_back(bottomLeft);
                    mesh.Indices.push_back(bottomRight);
                }
            }

            return mesh;
        }

        MeshData CreateQuad(float width, float height) {
            MeshData mesh;

            const float hw = width * 0.5f;
            const float hh = height * 0.5f;

            const glm::vec3 color(0.4f, 0.6f, 1.0f);

            mesh.Vertices = {
                {{-hw, -hh, 0.0f}, color, {0.0f, 0.0f}},
                {{hw, -hh, 0.0f},  color, {1.0f, 0.0f}},
                {{hw, hh, 0.0f},   color, {1.0f, 1.0f}},
                {{-hw, hh, 0.0f},  color, {0.0f, 1.0f}},
            };

            mesh.Indices = {0, 1, 2, 0, 2, 3};

            return mesh;
        }

        MeshData CreateCylinder(float radius, float height, uint32_t segments) {
            MeshData mesh;

            const float halfHeight = height * 0.5f;
            const float twoPi      = glm::two_pi<float>();

            // Side wall: two vertices (bottom, top) per column.
            for (uint32_t s = 0; s <= segments; ++s) {
                const float theta    = twoPi * static_cast<float>(s) / static_cast<float>(segments);
                const float sinTheta = glm::sin(theta);
                const float cosTheta = glm::cos(theta);

                const glm::vec3 normal(sinTheta, 0.0f, cosTheta);
                const glm::vec3 color = normal * 0.5f + 0.5f;

                Vertex bottom;
                bottom.Position = glm::vec3(sinTheta * radius, -halfHeight, cosTheta * radius);
                bottom.Color    = color;
                bottom.UV       = glm::vec2(static_cast<float>(s) / static_cast<float>(segments), 0.0f);

                Vertex top;
                top.Position = glm::vec3(sinTheta * radius, halfHeight, cosTheta * radius);
                top.Color    = color;
                top.UV       = glm::vec2(static_cast<float>(s) / static_cast<float>(segments), 1.0f);

                mesh.Vertices.push_back(bottom);
                mesh.Vertices.push_back(top);
            }

            for (uint32_t s = 0; s < segments; ++s) {
                const uint32_t bottom0 = s * 2;
                const uint32_t top0    = bottom0 + 1;
                const uint32_t bottom1 = bottom0 + 2;
                const uint32_t top1    = bottom0 + 3;

                mesh.Indices.push_back(bottom0);
                mesh.Indices.push_back(top0);
                mesh.Indices.push_back(bottom1);

                mesh.Indices.push_back(top0);
                mesh.Indices.push_back(top1);
                mesh.Indices.push_back(bottom1);
            }

            // Top cap: fan around the center vertex.
            const glm::vec3 topColor(0.5f, 0.8f, 0.5f);
            const uint32_t topCenter = static_cast<uint32_t>(mesh.Vertices.size());
            mesh.Vertices.push_back({
                {0.0f, halfHeight, 0.0f},
                topColor, {0.0f, 0.0f}
            });

            for (uint32_t s = 0; s <= segments; ++s) {
                const float theta = twoPi * static_cast<float>(s) / static_cast<float>(segments);
                mesh.Vertices.push_back({
                    {glm::sin(theta) * radius, halfHeight, glm::cos(theta) * radius},
                    topColor,
                    {static_cast<float>(s) / static_cast<float>(segments), 0.0f}
                });
            }

            for (uint32_t s = 0; s < segments; ++s) {
                mesh.Indices.push_back(topCenter);
                mesh.Indices.push_back(topCenter + 1 + s);
                mesh.Indices.push_back(topCenter + 1 + s + 1);
            }

            // Bottom cap: same fan, wound the other way.
            const glm::vec3 bottomColor(0.5f, 0.5f, 0.8f);
            const uint32_t bottomCenter = static_cast<uint32_t>(mesh.Vertices.size());
            mesh.Vertices.push_back({
                {0.0f, -halfHeight, 0.0f},
                bottomColor, {0.0f, 0.0f}
            });

            for (uint32_t s = 0; s <= segments; ++s) {
                const float theta = twoPi * static_cast<float>(s) / static_cast<float>(segments);
                mesh.Vertices.push_back({
                    {glm::sin(theta) * radius, -halfHeight, glm::cos(theta) * radius},
                    bottomColor,
                    {static_cast<float>(s) / static_cast<float>(segments), 0.0f}
                });
            }

            for (uint32_t s = 0; s < segments; ++s) {
                mesh.Indices.push_back(bottomCenter);
                mesh.Indices.push_back(bottomCenter + 1 + s + 1);
                mesh.Indices.push_back(bottomCenter + 1 + s);
            }

            return mesh;
        }

        MeshData CreateCapsule(float radius, float height, uint32_t segments, uint32_t rings) {
            MeshData mesh;

            struct Row {
                float Y;
                float RingRadius;
                float NormalY; // hemisphere axis tilt (1 = straight up, 0 = equator)
            };

            const float halfCylinder = glm::max(height - 2.0f * radius, 0.0f) * 0.5f;
            const uint32_t columns   = segments + 1;
            const float twoPi        = glm::two_pi<float>();

            std::vector<Row> rows;

            // Top pole.
            rows.push_back({halfCylinder + radius, 0.0f, 1.0f});

            // Top hemisphere (equator ring is added separately, matching the
            // cylinder rim).
            for (uint32_t k = 1; k <= rings; ++k) {
                const float phi = glm::half_pi<float>() * static_cast<float>(k) / static_cast<float>(rings);
                rows.push_back({halfCylinder + radius * glm::cos(phi), radius * glm::sin(phi), glm::cos(phi)});
            }

            // Cylinder rim rows (equator of the hemispheres).
            rows.push_back({halfCylinder, radius, 0.0f});
            if (halfCylinder > 0.0f) {
                rows.push_back({-halfCylinder, radius, 0.0f});
            }

            // Bottom hemisphere (mirrored).
            for (uint32_t k = rings; k >= 1; --k) {
                const float phi = glm::half_pi<float>() * static_cast<float>(k) / static_cast<float>(rings);
                rows.push_back({-halfCylinder - radius * glm::cos(phi), radius * glm::sin(phi), -glm::cos(phi)});
            }

            // Bottom pole.
            rows.push_back({-halfCylinder - radius, 0.0f, -1.0f});

            uint32_t rowIndex = 0;
            for (const Row& row : rows) {
                for (uint32_t s = 0; s <= segments; ++s) {
                    const float theta    = twoPi * static_cast<float>(s) / static_cast<float>(segments);
                    const float sinTheta = glm::sin(theta);
                    const float cosTheta = glm::cos(theta);

                    glm::vec3 normal;
                    if (row.NormalY != 0.0f) {
                        const float tilt = glm::sqrt(glm::max(1.0f - row.NormalY * row.NormalY, 0.0f));
                        normal           = glm::vec3(tilt * cosTheta, row.NormalY, tilt * sinTheta);
                    } else {
                        // Cylinder wall: horizontal normal.
                        normal = glm::vec3(cosTheta, 0.0f, sinTheta);
                    }

                    Vertex vertex;
                    vertex.Position = glm::vec3(cosTheta * row.RingRadius, row.Y, sinTheta * row.RingRadius);
                    vertex.Color    = normal * 0.5f + 0.5f;
                    vertex.UV       = glm::vec2(static_cast<float>(s) / static_cast<float>(segments),
                                                static_cast<float>(rowIndex) / static_cast<float>(rows.size() - 1));
                    mesh.Vertices.push_back(vertex);
                }
                ++rowIndex;
            }

            const uint32_t rowCount = static_cast<uint32_t>(rows.size());
            for (uint32_t r = 0; r + 1 < rowCount; ++r) {
                for (uint32_t s = 0; s < segments; ++s) {
                    const uint32_t a = r * columns + s;
                    const uint32_t b = a + 1;
                    const uint32_t c = (r + 1) * columns + s;
                    const uint32_t d = c + 1;

                    mesh.Indices.push_back(a);
                    mesh.Indices.push_back(c);
                    mesh.Indices.push_back(b);

                    mesh.Indices.push_back(b);
                    mesh.Indices.push_back(c);
                    mesh.Indices.push_back(d);
                }
            }

            return mesh;
        }

        MeshData CreateTorus(float majorRadius, float minorRadius, uint32_t segments, uint32_t rings) {
            MeshData mesh;

            const float twoPi = glm::two_pi<float>();

            // Ring rows around the major circle, each carrying a tube of
            // `segments` columns.
            for (uint32_t i = 0; i <= rings; ++i) {
                const float ringTheta = twoPi * static_cast<float>(i) / static_cast<float>(rings);
                const float cosMajor  = glm::cos(ringTheta);
                const float sinMajor  = glm::sin(ringTheta);

                const glm::vec3 ringCenter(cosMajor * majorRadius, 0.0f, sinMajor * majorRadius);

                for (uint32_t j = 0; j <= segments; ++j) {
                    const float tubePhi  = twoPi * static_cast<float>(j) / static_cast<float>(segments);
                    const float cosMinor = glm::cos(tubePhi);
                    const float sinMinor = glm::sin(tubePhi);

                    const glm::vec3 normal(cosMinor * cosMajor, sinMinor, cosMinor * sinMajor);

                    Vertex vertex;
                    vertex.Position = ringCenter + normal * minorRadius;
                    vertex.Color    = normal * 0.5f + 0.5f;
                    vertex.UV =
                        glm::vec2(static_cast<float>(j) / static_cast<float>(segments), static_cast<float>(i) / static_cast<float>(rings));
                    mesh.Vertices.push_back(vertex);
                }
            }

            for (uint32_t i = 0; i < rings; ++i) {
                for (uint32_t j = 0; j < segments; ++j) {
                    const uint32_t a = i * (segments + 1) + j;
                    const uint32_t b = a + 1;
                    const uint32_t c = (i + 1) * (segments + 1) + j;
                    const uint32_t d = c + 1;

                    mesh.Indices.push_back(a);
                    mesh.Indices.push_back(c);
                    mesh.Indices.push_back(b);

                    mesh.Indices.push_back(b);
                    mesh.Indices.push_back(c);
                    mesh.Indices.push_back(d);
                }
            }

            return mesh;
        }

        MeshData Create(PrimitiveType type) {
            switch (type) {
            case PrimitiveType::Cube:
                return CreateCube();
            case PrimitiveType::Quad:
                return CreateQuad();
            case PrimitiveType::Plane:
                return CreatePlane();
            case PrimitiveType::Sphere:
                return CreateSphere();
            case PrimitiveType::Cylinder:
                return CreateCylinder();
            case PrimitiveType::Capsule:
                return CreateCapsule();
            case PrimitiveType::Torus:
                return CreateTorus();
            }

            return CreateCube();
        }

    } // namespace internal


    MeshLibrary::MeshLibrary() {

        mMeshes.reserve(7);

        mMeshes[PrimitiveType::Cube]     = internal::CreateCube();
        mMeshes[PrimitiveType::Quad]     = internal::CreateQuad();
        mMeshes[PrimitiveType::Plane]    = internal::CreatePlane();
        mMeshes[PrimitiveType::Sphere]   = internal::CreateSphere();
        mMeshes[PrimitiveType::Cylinder] = internal::CreateCylinder();
        mMeshes[PrimitiveType::Capsule]  = internal::CreateCapsule();
        mMeshes[PrimitiveType::Torus]    = internal::CreateTorus();
    }

    MeshLibrary::~MeshLibrary() {
    }

    MeshData MeshLibrary::Get(PrimitiveType type) {
        return mMeshes[type];
    }


} // namespace golias
