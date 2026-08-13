#pragma once

#include "graphics/render_resources.h"
#include "scene/components/component.h"

namespace golias {

    // A component that holds a reference to a mesh resource. This component is used by the renderer to render the mesh in the scene.
    class MeshFilter final : public Component {
    public:

        void SetMesh(const Ref<Mesh>& mesh);

        const Ref<Mesh>& GetMesh() const;

    private:
        Ref<Mesh> mMesh = nullptr;
    };

} // namespace golias
