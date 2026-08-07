#include "graphics/rhi/vulkan/vk_renderpass.h"

#include "graphics/rhi/vulkan/vk_device.h"

namespace golias {
    void VulkanSubPass::AddColorAttachment(const VkAttachmentReference& attachment) {
        mColorAttachments.push_back(attachment);
    }

    void VulkanSubPass::AddInputAttachment(const VkAttachmentReference& attachment) {
        mInputAttachments.push_back(attachment);
    }

    void VulkanSubPass::SetDepthStencilAttachment(const VkAttachmentReference& attachment) {
        mDepthStencilAttachment = attachment;
    }

    void VulkanSubPass::SetResolveAttachment(const VkAttachmentReference& attachment) {
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

    const std::vector<VkAttachmentReference>& VulkanSubPass::GetColorAttachments() const {
        return mColorAttachments;
    }

    const std::vector<VkAttachmentReference>& VulkanSubPass::GetInputAttachments() const {
        return mInputAttachments;
    }

    const std::optional<VkAttachmentReference>& VulkanSubPass::GetDepthStencilAttachment() const {
        return mDepthStencilAttachment;
    }

    const std::optional<VkAttachmentReference>& VulkanSubPass::GetResolveAttachment() const {
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

        std::vector<VkSubpassDescription> subpassDescriptions;
        for (const auto& subpass : mSubpasses) {
            VkSubpassDescription subpassDescription{};
            subpassDescription.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpassDescription.colorAttachmentCount = static_cast<uint32_t>(subpass.GetColorAttachments().size());
            subpassDescription.pColorAttachments    = subpass.GetColorAttachments().data();
            subpassDescription.inputAttachmentCount = static_cast<uint32_t>(subpass.GetInputAttachments().size());
            subpassDescription.pInputAttachments    = subpass.GetInputAttachments().data();

            if (subpass.GetDepthStencilAttachment().has_value()) {
                subpassDescription.pDepthStencilAttachment = &subpass.GetDepthStencilAttachment().value();
            }

            if (subpass.GetResolveAttachment().has_value()) {
                subpassDescription.pResolveAttachments = &subpass.GetResolveAttachment().value();
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
