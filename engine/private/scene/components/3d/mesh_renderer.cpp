#include "scene/components/3d/mesh_renderer.h"


namespace golias {

    void MeshRenderer::SetMaterial(const Ref<MaterialInstance>& material) {
        mMaterial = material;
    }

    const Ref<MaterialInstance>& MeshRenderer::GetMaterial() const {
        return mMaterial;
    }

    void MeshRenderer::SetVisible(bool visible) {
        mVisible = visible;
    }

    bool MeshRenderer::IsVisible() const {
        return mVisible;
    }

    bool MeshRenderer::GetCastShadows() const {
        return mCastShadows;
    }

    void MeshRenderer::SetCastShadows(bool castShadows) {
        mCastShadows = castShadows;
    }

    bool MeshRenderer::GetReceiveShadows() const {
        return mReceiveShadows;
    }

    void MeshRenderer::SetReceiveShadows(bool receiveShadows) {
        mReceiveShadows = receiveShadows;
    }

} // namespace golias
