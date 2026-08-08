#pragma once
#include "graphics/rhi/rhi_texture.h"

namespace golias {

    enum class AttachmentLoadOp { Load, Clear, DontCare };

    enum class AttachmentStoreOp { Store, DontCare };

  
    enum class DepthStencilAccess {
        ReadWrite,       // Standard depth testing + write
        ReadOnlyDepth,   // Sample depth; stencil may still be written
        ReadOnlyStencil, // Depth may still be written; sample stencil
        ReadOnly,        // Both aspects read-only (depth sampling only)
    };


    struct DepthStencilAttachment {
        // The backing texture this view binds to. Nullptr means the render
        // pass has no depth/stencil attachment.
        Ref<Texture> Texture = nullptr;

        // Independent load/store behavior for the depth and stencil aspects.
        AttachmentLoadOp DepthLoadOp   = AttachmentLoadOp::Clear;
        AttachmentStoreOp DepthStoreOp = AttachmentStoreOp::Store;
        AttachmentLoadOp StencilLoadOp   = AttachmentLoadOp::DontCare;
        AttachmentStoreOp StencilStoreOp = AttachmentStoreOp::DontCare;

        // Values used when the matching LoadOp is Clear.
        float DepthClearValue     = 1.0f;
        uint32_t StencilClearValue = 0;

        // Read/write policy for the two aspects (maps to a native image layout on the backend).
        DepthStencilAccess Access = DepthStencilAccess::ReadWrite;
    };

} // namespace golias
