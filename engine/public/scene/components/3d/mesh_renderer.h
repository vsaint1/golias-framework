#pragma once

#include "graphics/render_resources.h"
#include "scene/components/component.h"

namespace golias {

    // A component that holds a reference to a material instance. This component is used by the renderer to render the mesh with the specified material in the scene.
    class MeshRenderer final : public Component {
    public:
        void SetMaterial(const Ref<MaterialInstance>& material);

        const Ref<MaterialInstance>& GetMaterial() const;

        void SetVisible(bool visible);

        bool IsVisible() const;

        bool GetCastShadows() const;

        void SetCastShadows(bool castShadows);

        bool GetReceiveShadows() const;

        void SetReceiveShadows(bool receiveShadows);

    private:
        Ref<MaterialInstance> mMaterial = nullptr;

        bool mVisible = true;

        bool mCastShadows = true;

        bool mReceiveShadows = true;
    };

} // namespace golias
