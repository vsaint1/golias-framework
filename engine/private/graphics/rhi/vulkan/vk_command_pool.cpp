#include "graphics/rhi/vulkan/vk_command_pool.h"

#include "graphics/rhi/vulkan/vk_device.h"

namespace golias {

    VulkanCommandPool::VulkanCommandPool(Ref<VulkanDevice> device, VkCommandPoolCreateFlags flags) : mDevice(device) {

        VkCommandPoolCreateInfo poolInfo = {.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                                            .pNext            = nullptr,
                                            .flags            = flags,
                                            .queueFamilyIndex = mDevice->GetGraphicsQueueFamilyIndex()};

        if (vkCreateCommandPool(mDevice->GetHandle(), &poolInfo, nullptr, &mCommandPool) != VK_SUCCESS) {
            LOG_FATAL("Failed to create command pool.");
        }

        LOG_INFO("Created command pool with handle: {:#x}", reinterpret_cast<uint64_t>(mCommandPool));
    }

    VulkanCommandPool::~VulkanCommandPool() {
        vkDestroyCommandPool(mDevice->GetHandle(), mCommandPool, nullptr);
    }

} // namespace golias
