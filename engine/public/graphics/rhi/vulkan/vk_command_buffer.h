#pragma once
#include "vk_common.h"
#include "graphics/rhi/rhi_buffer.h"
#include "graphics/rhi/rhi_graphics_pipeline.h"
#include "graphics/rhi/rhi_uniform_buffer.h"

namespace golias {

    class VulkanDevice;
    class VulkanCommandPool;

    class VulkanCommandBuffer {
    public:
        VulkanCommandBuffer(Ref<VulkanDevice> device, Ref<VulkanCommandPool> commandPool);
        ~VulkanCommandBuffer();

        void Begin(VkCommandBufferUsageFlags flags = 0);
        void End();

        void BeginRenderPass(const VkRenderPassBeginInfo& renderPassInfo, VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);
        void EndRenderPass();

        void BindGraphicsPipeline(const Ref<GraphicsPipeline>& pipeline);

        void BindUniformBufferSet(const Ref<UniformBufferSet>& uniformBufferSet, uint32_t frameIndex);

        void BindVertexBuffer(const Ref<Buffer>& buffer);

        void BindIndexBuffer(const Ref<Buffer>& buffer);

        void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0);

        void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0);

        void SetViewport(const VkViewport& viewport);
        void SetScissor(const VkRect2D& scissor);

        void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0);

        void Submit(VkQueue queue);

        void Wait(VkQueue queue);

        VkCommandBuffer GetHandle() const {
            return mCommandBuffer;
        }

    private:
        Ref<VulkanDevice> mDevice           = nullptr;
        VkCommandBuffer mCommandBuffer      = VK_NULL_HANDLE;
        Ref<VulkanCommandPool> mCommandPool = nullptr;

        VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
    };

} // namespace golias
