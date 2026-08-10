#pragma once
#include "stdafx.h"

#include <glm/glm.hpp>

namespace golias {


    struct Vertex {
        glm::vec3 Position;
        glm::vec3 Color;
        glm::vec2 UV;
    };


    struct MeshData {
        std::vector<Vertex> Vertices;
        std::vector<uint32_t> Indices;

        bool IsValid() const {
            return !Vertices.empty() && !Indices.empty();
        }
    };

    // Built-in primitives the mesh library can generate procedurally.
    enum class PrimitiveType {
        Cube, // unit cube, per-face colors
        Quad, // axis-aligned quad on the XY plane, facing +Z
        Plane, // axis-aligned plane on the XZ plane
        Sphere, // UV sphere
        Cylinder, // axis-aligned cylinder on Y (with caps)
        Capsule, // Unity-style capsule (cylinder + hemispheres) on Y
        Torus, // ring lying on the XZ plane
    };

    class MeshLibrary {
    public:
        MeshLibrary();
        ~MeshLibrary();

        MeshData Get(PrimitiveType type);

    private:
        std::unordered_map<PrimitiveType, MeshData> mMeshes = {};
    };

    namespace internal {

        // Unit cube (size 1) centered at the origin.
        MeshData CreateCube(float size = 1.0f);

        // Axis-aligned quad on the XY plane, centered, facing +Z.
        MeshData CreateQuad(float width = 1.0f, float height = 1.0f);

        // Axis-aligned plane on the XZ plane, centered, subdivided into
        // segmentsX x segmentsZ tiles.
        MeshData CreatePlane(float width = 1.0f, float depth = 1.0f, uint32_t segmentsX = 1, uint32_t segmentsZ = 1);

        // UV sphere with `segments` horizontal steps and `rings` vertical
        // steps. Color is derived from the surface normal.
        MeshData CreateSphere(float radius = 0.5f, uint32_t segments = 24, uint32_t rings = 16);

        // Cylinder on the Y axis with capped ends, `segments` side columns.
        MeshData CreateCylinder(float radius = 0.5f, float height = 1.0f, uint32_t segments = 24);

        // Unity-style capsule on the Y axis: a cylinder of height
        // `height - 2 * radius` (0 if negative) capped with two hemispheres.
        MeshData CreateCapsule(float radius = 0.5f, float height = 1.0f, uint32_t segments = 24, uint32_t rings = 8);

        // Torus lying on the XZ plane.
        MeshData CreateTorus(float majorRadius = 0.6f, float minorRadius = 0.2f, uint32_t segments = 32, uint32_t rings = 16);

        // Dispatcher for the built-in primitives (default parameters).
        MeshData Create(PrimitiveType type);

    } // namespace internal

} // namespace golias
