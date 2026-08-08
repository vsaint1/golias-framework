#include "graphics/rhi/vulkan/vk_renderpass.h"

#include "graphics/rhi/vulkan/vk_device.h"
#include "graphics/rhi/vulkan/vk_texture.h"

namespace golias {

    namespace {

        VkAttachmentLoadOp ToVulkanLoadOp(AttachmentLoadOp op) {
            switch (op) {
            case AttachmentLoadOp::Load:
                return VK_ATTACHMENT_LOAD_OP_LOAD;
            case AttachmentLoadOp::Clear:
                return VK_ATTACHMENT_LOAD_OP_CLEAR;
            case AttachmentLoadOp::DontCare:
                return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            }

            return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        }

        VkAttachmentStoreOp ToVulkanStoreOp(AttachmentStoreOp op) {
            switch (op) {
            case AttachmentStoreOp::Store:
                return VK_ATTACHMENT_STORE_OP_STORE;
            case AttachmentStoreOp::DontCare:
                return VK_ATTACHMENT_STORE_OP_DONT_CARE;
            }

            return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }

        // Maps an RHI depth/stencil access mode to the native image layout
        VkImageLayout ToVulkanDepthStencilLayout(DepthStencilAccess access) {
            switch (access) {
            case DepthStencilAccess::ReadOnlyDepth:
                return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
            case DepthStencilAccess::ReadOnlyStencil:
                return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
            case DepthStencilAccess::ReadOnly:
                return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            case DepthStencilAccess::ReadWrite:
            default:
                return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }
        }

        // Maps an RHI attachment layout to the native image layout.
        VkImageLayout ToVulkanImageLayout(AttachmentLayout layout) {
            switch (layout) {
            case AttachmentLayout::General:
                return VK_IMAGE_LAYOUT_GENERAL;
            case AttachmentLayout::ColorAttachmentOptimal:
                return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            case AttachmentLayout::DepthStencilAttachmentOptimal:
                return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            case AttachmentLayout::DepthReadOnlyStencilAttachmentOptimal:
                return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
            case AttachmentLayout::DepthAttachmentStencilReadOnlyOptimal:
                return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
            case AttachmentLayout::DepthStencilReadOnlyOptimal:
                return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            case AttachmentLayout::ShaderReadOnlyOptimal:
                return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            case AttachmentLayout::PresentSource:
                return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            case AttachmentLayout::Undefined:
            default:
                return VK_IMAGE_LAYOUT_UNDEFINED;
            }
        }

        VkAttachmentReference ToVulkanAttachmentReference(const AttachmentReference& reference) {
            return {
                .attachment = reference.Attachment,
                .layout     = ToVulkanImageLayout(reference.Layout),
            };
        }

    } // namespace


    void VulkanSubPass::AddColorAttachment(const AttachmentReference& attachment) {
        mColorAttachments.push_back(attachment);
    }

    void VulkanSubPass::AddInputAttachment(const AttachmentReference& attachment) {
        mInputAttachments.push_back(attachment);
    }

    void VulkanSubPass::SetDepthStencilAttachment(const AttachmentReference& attachment) {
        mDepthStencilAttachment = attachment;
    }

    void VulkanSubPass::SetResolveAttachment(const AttachmentReference& attachment) {
        mResolveAttachment = attachment;
    }

    void VulkanSubPass::Validate() const {
        if (mColorAttachments.empty()) {
            LOG_FATAL("SubPass must have at least one color attachment.");
        }

        if (mDepthStencilAttachment.has_value() && mResolveAttachment.has_value()) {
            LOG_FATAL("SubPass cannot have both depth/stencil and resolve attachments.");
        }
    }

    const std::vector<AttachmentReference>& VulkanSubPass::GetColorAttachments() const {
        return mColorAttachments;
    }

    const std::vector<AttachmentReference>& VulkanSubPass::GetInputAttachments() const {
        return mInputAttachments;
    }

    const std::optional<AttachmentReference>& VulkanSubPass::GetDepthStencilAttachment() const {
        return mDepthStencilAttachment;
    }

    const std::optional<AttachmentReference>& VulkanSubPass::GetResolveAttachment() const {
        return mResolveAttachment;
    }


    VulkanRenderPass::VulkanRenderPass(Ref<VulkanDevice> device) : mDevice(device) {
    }

    VulkanRenderPass::~VulkanRenderPass() {
        if (mRenderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(mDevice->GetHandle(), mRenderPass, nullptr);
        }
    }

    void VulkanRenderPass::AddAttachment(const VkAttachmentDescription& attachment) {
        mAttachmentDescriptions.push_back(attachment);
    }

    void VulkanRenderPass::AddAttachment(const DepthStencilAttachment& attachment) {
        if (!attachment.Texture) {
            return;
        }

        auto* vulkanTexture = static_cast<VulkanTexture*>(attachment.Texture.get());

        VkAttachmentDescription description = {
            .format         = vulkanTexture->GetVulkanFormat(),
            .samples        = static_cast<VkSampleCountFlagBits>(vulkanTexture->GetSampleCount()),
            .loadOp         = ToVulkanLoadOp(attachment.DepthLoadOp),
            .storeOp        = ToVulkanStoreOp(attachment.DepthStoreOp),
            .stencilLoadOp  = ToVulkanLoadOp(attachment.StencilLoadOp),
            .stencilStoreOp = ToVulkanStoreOp(attachment.StencilStoreOp),
            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = ToVulkanDepthStencilLayout(attachment.Access),
        };

        mAttachmentDescriptions.push_back(description);
    }

    void VulkanRenderPass::AddSubPass(const VulkanSubPass& subpass) {
        mSubpasses.push_back(subpass);
    }

    void VulkanRenderPass::AddDependency(const VkSubpassDependency& dependency) {
        mDependencies.push_back(dependency);
    }

    void VulkanRenderPass::Build() {
        if (mSubpasses.empty() || mAttachmentDescriptions.empty()) {
            LOG_FATAL("RenderPass must have at least one subpass and one attachment.");
        }

        for (const auto& subpass : mSubpasses) {
            subpass.Validate();
        }

        // Translate each subpass's RHI references into native ones.
        std::vector<std::vector<VkAttachmentReference>> colorReferencesBySubpass;
        std::vector<std::vector<VkAttachmentReference>> inputReferencesBySubpass;
        std::vector<VkAttachmentReference> depthStencilReferencesBySubpass;
        std::vector<VkAttachmentReference> resolveReferencesBySubpass;

        colorReferencesBySubpass.reserve(mSubpasses.size());
        inputReferencesBySubpass.reserve(mSubpasses.size());
        depthStencilReferencesBySubpass.reserve(mSubpasses.size());
        resolveReferencesBySubpass.reserve(mSubpasses.size());

        for (const auto& subpass : mSubpasses) {
            std::vector<VkAttachmentReference> colorReferences;
            colorReferences.reserve(subpass.GetColorAttachments().size());
            for (const AttachmentReference& reference : subpass.GetColorAttachments()) {
                colorReferences.push_back(ToVulkanAttachmentReference(reference));
            }
            colorReferencesBySubpass.push_back(std::move(colorReferences));

            std::vector<VkAttachmentReference> inputReferences;
            inputReferences.reserve(subpass.GetInputAttachments().size());
            for (const AttachmentReference& reference : subpass.GetInputAttachments()) {
                inputReferences.push_back(ToVulkanAttachmentReference(reference));
            }
            inputReferencesBySubpass.push_back(std::move(inputReferences));

            depthStencilReferencesBySubpass.push_back(subpass.GetDepthStencilAttachment().has_value()
                                                          ? ToVulkanAttachmentReference(subpass.GetDepthStencilAttachment().value())
                                                          : VkAttachmentReference{});
            resolveReferencesBySubpass.push_back(subpass.GetResolveAttachment().has_value()
                                                     ? ToVulkanAttachmentReference(subpass.GetResolveAttachment().value())
                                                     : VkAttachmentReference{});
        }

        std::vector<VkSubpassDescription> subpassDescriptions;
        subpassDescriptions.reserve(mSubpasses.size());

        for (size_t i = 0; i < mSubpasses.size(); ++i) {
            VkSubpassDescription subpassDescription{};
            subpassDescription.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpassDescription.colorAttachmentCount = static_cast<uint32_t>(colorReferencesBySubpass[i].size());
            subpassDescription.pColorAttachments    = colorReferencesBySubpass[i].data();
            subpassDescription.inputAttachmentCount = static_cast<uint32_t>(inputReferencesBySubpass[i].size());
            subpassDescription.pInputAttachments    = inputReferencesBySubpass[i].data();

            if (mSubpasses[i].GetDepthStencilAttachment().has_value()) {
                subpassDescription.pDepthStencilAttachment = &depthStencilReferencesBySubpass[i];
            }

            if (mSubpasses[i].GetResolveAttachment().has_value()) {
                subpassDescription.pResolveAttachments = &resolveReferencesBySubpass[i];
            }

            subpassDescriptions.push_back(subpassDescription);
        }

        VkRenderPassCreateInfo renderPassInfo = {
            .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .pNext           = nullptr,
            .attachmentCount = static_cast<uint32_t>(mAttachmentDescriptions.size()),
            .pAttachments    = mAttachmentDescriptions.data(),
            .subpassCount    = static_cast<uint32_t>(subpassDescriptions.size()),
            .pSubpasses      = subpassDescriptions.data(),
            .dependencyCount = static_cast<uint32_t>(mDependencies.size()),
            .pDependencies   = mDependencies.data(),
        };

        if (vkCreateRenderPass(mDevice->GetHandle(), &renderPassInfo, nullptr, &mRenderPass) != VK_SUCCESS) {
            LOG_FATAL("Failed to create render pass.");
        }

        LOG_INFO("RenderPass created successfully.");
    }

} // namespace golias
