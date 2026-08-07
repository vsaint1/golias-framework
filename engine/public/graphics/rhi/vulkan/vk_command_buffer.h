#pragma once
#include "vk_common.h"

namespace golias {

    class VulkanDevice;
    class VulkanPipeline;
    class VulkanCommandPool;

    class VulkanCommandBuffer {
    public:
        VulkanCommandBuffer(Ref<VulkanDevice> device, Ref<VulkanCommandPool> commandPool);
        ~VulkanCommandBuffer();

        void Begin(VkCommandBufferUsageFlags flags = 0);
        void End();

        void BeginRenderPass(const VkRenderPassBeginInfo& renderPassInfo, VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);
        void EndRenderPass();

        void BindGraphicsPipeline(const Ref<VulkanPipeline>& pipeline);

        void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0);

        VkCommandBuffer GetHandle() const {
            return mCommandBuffer;
        }

    private:
        Ref<VulkanDevice> mDevice           = nullptr;
        VkCommandBuffer mCommandBuffer      = VK_NULL_HANDLE;
        Ref<VulkanCommandPool> mCommandPool = nullptr;
    };

} // namespace golias
