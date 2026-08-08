#include "graphics/rhi/vulkan/vk_command_buffer.h"

#include "graphics/rhi/vulkan/vk_buffer.h"
#include "graphics/rhi/vulkan/vk_command_pool.h"
#include "graphics/rhi/vulkan/vk_device.h"
#include "graphics/rhi/vulkan/vk_graphics_pipeline.h"
#include "graphics/rhi/vulkan/vk_uniform_buffer.h"

namespace golias {

    VulkanCommandBuffer::VulkanCommandBuffer(Ref<VulkanDevice> device, Ref<VulkanCommandPool> commandPool)
        : mDevice(device), mCommandPool(commandPool) {

        VkCommandBufferAllocateInfo allocInfo = {

            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = commandPool->GetHandle(),
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1};

        if (vkAllocateCommandBuffers(mDevice->GetHandle(), &allocInfo, &mCommandBuffer) != VK_SUCCESS) {
            LOG_WARN("Failed to allocate command buffer!");
        }

        LOG_INFO("Allocated command buffer: {}", (void*) mCommandBuffer);
    }

    void VulkanCommandBuffer::Begin(VkCommandBufferUsageFlags flags) {
        VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = flags,
        };

        vkBeginCommandBuffer(mCommandBuffer, &beginInfo);
    }

    void VulkanCommandBuffer::End() {
        vkEndCommandBuffer(mCommandBuffer);
    }

    void VulkanCommandBuffer::SetViewport(const VkViewport& viewport) {
        vkCmdSetViewport(mCommandBuffer, 0, 1, &viewport);
    }

    void VulkanCommandBuffer::SetScissor(const VkRect2D& scissor) {
        vkCmdSetScissor(mCommandBuffer, 0, 1, &scissor);
    }

    void VulkanCommandBuffer::CopyBuffer(
        VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize srcOffset, VkDeviceSize dstOffset) {
        VkBufferCopy copyRegion = {
            .srcOffset = srcOffset,
            .dstOffset = dstOffset,
            .size      = size,
        };

        vkCmdCopyBuffer(mCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    }

    void VulkanCommandBuffer::Submit(VkQueue queue) {
        VkSubmitInfo submitInfo = {
            .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers    = &mCommandBuffer,
        };

        VkResult result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        VK_CHECK_RESULT(result);
    }

    void VulkanCommandBuffer::Wait(VkQueue queue) {
        VkResult result = vkQueueWaitIdle(queue);
        VK_CHECK_RESULT(result);
    }

    void VulkanCommandBuffer::BeginRenderPass(const VkRenderPassBeginInfo& renderPassInfo, VkSubpassContents contents) {
        vkCmdBeginRenderPass(mCommandBuffer, &renderPassInfo, contents);
    }

    void VulkanCommandBuffer::EndRenderPass() {
        vkCmdEndRenderPass(mCommandBuffer);
    }

    void VulkanCommandBuffer::BindGraphicsPipeline(const Ref<GraphicsPipeline>& pipeline) {
        auto vkPipeline = static_pointer_cast<VulkanGraphicsPipeline>(pipeline);
        mPipelineLayout = vkPipeline->GetLayout();
        vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline->GetHandle());
    }

    void VulkanCommandBuffer::BindUniformBufferSet(const Ref<UniformBufferSet>& uniformBufferSet, uint32_t frameIndex) {
        auto vkUniformBufferSet = static_pointer_cast<VulkanUniformBufferSet>(uniformBufferSet);

        VkPipelineLayout pipelineLayout = mPipelineLayout;
        if (pipelineLayout == VK_NULL_HANDLE) {
            LOG_FATAL("Cannot bind uniform buffer set: no pipeline bound yet!");
        }

        VkDescriptorSet descriptorSet = vkUniformBufferSet->GetDescriptorSet(frameIndex);
        vkCmdBindDescriptorSets(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
    }

    void VulkanCommandBuffer::BindVertexBuffer(const Ref<Buffer>& buffer) {
        auto vkBuffer = static_pointer_cast<VulkanBuffer>(buffer);

        VkBuffer vertexBuffers[] = {vkBuffer->GetHandle()};
        VkDeviceSize offsets[]   = {0};
        vkCmdBindVertexBuffers(mCommandBuffer, 0, 1, vertexBuffers, offsets);
    }

    void VulkanCommandBuffer::BindIndexBuffer(const Ref<Buffer>& buffer) {
        auto vkBuffer = static_pointer_cast<VulkanBuffer>(buffer);
        vkCmdBindIndexBuffer(mCommandBuffer, vkBuffer->GetHandle(), 0, VK_INDEX_TYPE_UINT32);
    }

    void VulkanCommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
        vkCmdDraw(mCommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void VulkanCommandBuffer::DrawIndexed(
        uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
        vkCmdDrawIndexed(mCommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    VulkanCommandBuffer::~VulkanCommandBuffer() {
        if (mCommandBuffer != VK_NULL_HANDLE) {
            vkFreeCommandBuffers(mDevice->GetHandle(), mCommandPool->GetHandle(), 1, &mCommandBuffer);
            mCommandBuffer = VK_NULL_HANDLE;
        }
    }

} // namespace golias
