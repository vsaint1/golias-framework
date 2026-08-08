#pragma once
#include "stdafx.h"

#include "scene/components/component.h"
#include "graphics/rhi/rhi_buffer.h"

#include <glm/glm.hpp>

namespace golias {


    class MeshRenderer : public Component {
    public:
        MeshRenderer() = default;
        ~MeshRenderer() override = default;

        void SetBuffers(const Ref<Buffer>& vertexBuffer, const Ref<Buffer>& indexBuffer, uint32_t indexCount);

        const Ref<Buffer>& GetVertexBuffer() const;

        const Ref<Buffer>& GetIndexBuffer() const;

        uint32_t GetIndexCount() const;

        // Convenience: world transform of the owning GameObject.
        glm::mat4 GetModelMatrix() const;

    private:
        Ref<Buffer> mVertexBuffer = nullptr;
        Ref<Buffer> mIndexBuffer  = nullptr;
        uint32_t mIndexCount      = 0;
    };

} // namespace golias
