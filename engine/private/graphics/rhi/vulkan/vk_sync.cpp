#include "graphics/rhi/vulkan/vk_sync.h"
#include "graphics/rhi/vulkan/vk_device.h"

namespace golias {

    VulkanFence::VulkanFence(Ref<VulkanDevice> device, bool signaled) : mDevice(device) {
        VkFenceCreateInfo fenceInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : (VkFenceCreateFlags)0,
        };

        VkResult result = vkCreateFence(mDevice->GetHandle(), &fenceInfo, nullptr, &mFence);
        VK_CHECK_RESULT(result);

        LOG_INFO("Created fence with handle {} | Signaled: {}", (void*)mFence, signaled);
    }

    void VulkanFence::Wait() const {
        VkResult result = vkWaitForFences(mDevice->GetHandle(), 1, &mFence, VK_TRUE, UINT64_MAX);
        VK_CHECK_RESULT(result);
    }

    void VulkanFence::Reset() const {
        VkResult result = vkResetFences(mDevice->GetHandle(), 1, &mFence);
        VK_CHECK_RESULT(result);
    }

    VulkanFence::~VulkanFence() {
        if (mFence != VK_NULL_HANDLE) {
            vkDestroyFence(mDevice->GetHandle(), mFence, nullptr);
            mFence = VK_NULL_HANDLE;
        }
    }

    VkFence VulkanFence::GetHandle() const {
        return mFence;
    }

    VulkanSemaphore::VulkanSemaphore(Ref<VulkanDevice> device) : mDevice(device) {
        VkSemaphoreCreateInfo semaphoreInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };

        VkResult result = vkCreateSemaphore(mDevice->GetHandle(), &semaphoreInfo, nullptr, &mSemaphore);
        VK_CHECK_RESULT(result);

        LOG_INFO("Created semaphore with handle {}", (void*)mSemaphore);
    }

    VulkanSemaphore::~VulkanSemaphore() {
        if (mSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(mDevice->GetHandle(), mSemaphore, nullptr);
            mSemaphore = VK_NULL_HANDLE;
        }
    }

    VkSemaphore VulkanSemaphore::GetHandle() const {
        return mSemaphore;
    }

} // namespace golias