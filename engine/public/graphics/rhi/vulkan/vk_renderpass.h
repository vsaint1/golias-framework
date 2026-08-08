#pragma once
#include "graphics/rhi/rhi_renderpass.h"
#include "vk_common.h"

namespace golias {
    class VulkanRenderPass;

    class VulkanSubPass {
    public:
        VulkanSubPass()  = default;
        ~VulkanSubPass() = default;

        void AddColorAttachment(const AttachmentReference& attachment);

        void AddInputAttachment(const AttachmentReference& attachment);

        void SetDepthStencilAttachment(const AttachmentReference& attachment);

        void SetResolveAttachment(const AttachmentReference& attachment);

        const std::vector<AttachmentReference>& GetColorAttachments() const;

        const std::vector<AttachmentReference>& GetInputAttachments() const;

        const std::optional<AttachmentReference>& GetDepthStencilAttachment() const;

        const std::optional<AttachmentReference>& GetResolveAttachment() const;

        void Validate() const;

    private:
        std::vector<AttachmentReference> mColorAttachments         = {};
        std::vector<AttachmentReference> mInputAttachments         = {};
        std::optional<AttachmentReference> mDepthStencilAttachment = std::nullopt;
        std::optional<AttachmentReference> mResolveAttachment      = std::nullopt;
    };

    class VulkanDevice;


    class VulkanRenderPass {
    public:
        explicit VulkanRenderPass(Ref<VulkanDevice> device);
        ~VulkanRenderPass();

        void AddAttachment(const VkAttachmentDescription& attachment);

        // translates a DepthStencilAttachment view
        // descriptor (texture + load/store ops + access) into a native
        // attachment description.
        void AddAttachment(const DepthStencilAttachment& attachment);

        void AddSubPass(const VulkanSubPass& subpass);

        void AddDependency(const VkSubpassDependency& dependency);

        void Build();


        VkRenderPass GetHandle() const {
            return mRenderPass;
        }


    private:
        Ref<VulkanDevice> mDevice;
        VkRenderPass mRenderPass = VK_NULL_HANDLE;

        std::vector<VkAttachmentDescription> mAttachmentDescriptions = {};
        std::vector<VulkanSubPass> mSubpasses                        = {};
        std::vector<VkSubpassDependency> mDependencies               = {};
    };
} // namespace golias
