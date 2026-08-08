#include "scene/components/3d/mesh_renderer.h"

#include "scene/game_object.h"

namespace golias {

    void MeshRenderer::SetBuffers(const Ref<Buffer>& vertexBuffer, const Ref<Buffer>& indexBuffer, uint32_t indexCount) {
        mVertexBuffer = vertexBuffer;
        mIndexBuffer  = indexBuffer;
        mIndexCount   = indexCount;
    }

    const Ref<Buffer>& MeshRenderer::GetVertexBuffer() const {
        return mVertexBuffer;
    }

    const Ref<Buffer>& MeshRenderer::GetIndexBuffer() const {
        return mIndexBuffer;
    }

    uint32_t MeshRenderer::GetIndexCount() const {
        return mIndexCount;
    }

    glm::mat4 MeshRenderer::GetModelMatrix() const {
        GameObject* owner = GetOwner();
        return owner != nullptr ? owner->GetWorldTransform() : glm::mat4(1.0f);
    }

} // namespace golias
