#pragma once
#include "vk_common.h"

namespace golias {
    class VulkanRenderPass;

    class VulkanSubPass {
    public:
        VulkanSubPass()  = default;
        ~VulkanSubPass() = default;

        void AddColorAttachment(const VkAttachmentReference& attachment);

        void AddInputAttachment(const VkAttachmentReference& attachment);

        void SetDepthStencilAttachment(const VkAttachmentReference& attachment);

        void SetResolveAttachment(const VkAttachmentReference& attachment);

        const std::vector<VkAttachmentReference>& GetColorAttachments() const;

        const std::vector<VkAttachmentReference>& GetInputAttachments() const;
        
        const std::optional<VkAttachmentReference>& GetDepthStencilAttachment() const;

        const std::optional<VkAttachmentReference>& GetResolveAttachment() const;
        
        void Validate() const;

    private:
        std::vector<VkAttachmentReference> mColorAttachments         = {};
        std::vector<VkAttachmentReference> mInputAttachments         = {};
        std::optional<VkAttachmentReference> mDepthStencilAttachment = std::nullopt;
        std::optional<VkAttachmentReference> mResolveAttachment      = std::nullopt;
    };

    class VulkanDevice;

    class VulkanRenderPass {
    public:
        explicit VulkanRenderPass(Ref<VulkanDevice> device);
        ~VulkanRenderPass();

        void AddAttachment(const VkAttachmentDescription& attachment);

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
        std::vector<VulkanSubPass> mSubpasses                              = {};
        std::vector<VkSubpassDependency> mDependencies               = {};
    };
} // namespace golias
