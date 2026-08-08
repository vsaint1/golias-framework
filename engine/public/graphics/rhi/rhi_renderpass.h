#pragma once
#include "stdafx.h"

#include "graphics/rhi/rhi_depth_stencil_attachment.h"

namespace golias {

    // Backend-independent image layout used by attachment references and
    // attachments. 
    enum class AttachmentLayout {
        Undefined,
        General,
        ColorAttachmentOptimal,
        DepthStencilAttachmentOptimal,
        DepthReadOnlyStencilAttachmentOptimal,
        DepthAttachmentStencilReadOnlyOptimal,
        DepthStencilReadOnlyOptimal,
        ShaderReadOnlyOptimal,
        PresentSource,
    };

    // Maps a depth/stencil access mode to the matching attachment layout. Use
    // for the depth/stencil attachment reference so the layout always agrees
    // with the access policy described by the DepthStencilAttachment.
    inline AttachmentLayout ToAttachmentLayout(DepthStencilAccess access) {
        switch (access) {
        case DepthStencilAccess::ReadOnlyDepth:
            return AttachmentLayout::DepthReadOnlyStencilAttachmentOptimal;
        case DepthStencilAccess::ReadOnlyStencil:
            return AttachmentLayout::DepthAttachmentStencilReadOnlyOptimal;
        case DepthStencilAccess::ReadOnly:
            return AttachmentLayout::DepthStencilReadOnlyOptimal;
        case DepthStencilAccess::ReadWrite:
        default:
            return AttachmentLayout::DepthStencilAttachmentOptimal;
        }
    }

    // RHI attachment reference: which attachment (by index) a subpass uses and
    // in what image layout.
    struct AttachmentReference {
        uint32_t Attachment  = 0;
        AttachmentLayout Layout = AttachmentLayout::Undefined;
    };

} // namespace golias
