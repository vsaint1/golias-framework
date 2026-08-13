#pragma once

#include "graphics/render_resources.h"

namespace golias {

    class MeshLibrary {
    public:
        static Ref<Mesh> CreateCube(const Ref<RHIDevice>& device);
        static Ref<Mesh> CreatePlane(const Ref<RHIDevice>& device);
        static Ref<Mesh> CreateQuad(const Ref<RHIDevice>& device);
        static Ref<Mesh> CreateSphere(const Ref<RHIDevice>& device, uint32_t segments = 24, uint32_t rings = 16);
        static Ref<Mesh> CreateCylinder(const Ref<RHIDevice>& device, uint32_t segments = 24);
        static Ref<Mesh> CreateCapsule(const Ref<RHIDevice>& device, uint32_t segments = 24, uint32_t rings = 12);
        static Ref<Mesh> CreateTorus(const Ref<RHIDevice>& device, uint32_t segments = 32, uint32_t rings = 16);
    };

} // namespace golias
