#pragma once
#include "vk_common.h"


namespace golias {

    class VulkanDevice;

    class VulkanCommandPool {
    public:
        VulkanCommandPool(Ref<VulkanDevice> device,  VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        ~VulkanCommandPool();

        VkCommandPool GetHandle() const {
            return mCommandPool;
        }

    private:
        Ref<VulkanDevice> mDevice = nullptr;
        VkCommandPool mCommandPool = VK_NULL_HANDLE;
    };
} // namespace golias
