#include "graphics/rhi/vulkan/vk_device.h"

#include "graphics/rhi/vulkan/vk_instance.h"
#include "graphics/rhi/vulkan/vk_window_surface.h"


namespace golias {

    VulkanDevice::VulkanDevice(Ref<VulkanInstance> instance, Ref<VulkanWindowSurface> surface) : mInstance(instance), mSurface(surface) {
        PickPhysicalDevice();
        SetupDeviceQueues();
        CreateLogicalDevice();
    }

    VulkanDevice::~VulkanDevice() {
        if(mDevice != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(mDevice);
            vkDestroyDevice(mDevice, nullptr);
        }
    }

    void VulkanDevice::PickPhysicalDevice() {
        uint32_t deviceCount = 0;
        VkResult result      = vkEnumeratePhysicalDevices(mInstance->GetHandle(), &deviceCount, nullptr);
        VK_CHECK_RESULT(result);

        if (deviceCount == 0) {
            LOG_FATAL("Failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        result = vkEnumeratePhysicalDevices(mInstance->GetHandle(), &deviceCount, devices.data());
        VK_CHECK_RESULT(result);

        mPhysicalDevice = VK_NULL_HANDLE;

        for (const VkPhysicalDevice& device : devices) {

            bool hasSwapchainExtension = false;

            uint32_t extensionCount = 0;
            result                  = vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
            VK_CHECK_RESULT(result);

            std::vector<VkExtensionProperties> extensions(extensionCount);
            result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());
            VK_CHECK_RESULT(result);

            for (const auto& extension : extensions) {
                if (strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
                    hasSwapchainExtension = true;
                    break;
                }
            }

            if (!hasSwapchainExtension) {
                continue;
            }


            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

            bool hasGraphicsPresentQueue = false;

            for (uint32_t i = 0; i < queueFamilyCount; ++i) {
                if (!(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                    continue;
                }

                VkBool32 presentSupport = VK_FALSE;
                result                  = vkGetPhysicalDeviceSurfaceSupportKHR(device, i, mSurface->GetHandle(), &presentSupport);
                VK_CHECK_RESULT(result);

                if (presentSupport) {
                    mGraphicsQueueFamilyIndex = i;
                    mPresentQueueFamilyIndex  = i;
                    hasGraphicsPresentQueue   = true;
                    break;
                }
            }

            if (!hasGraphicsPresentQueue) {
                continue;
            }

            mPhysicalDevice = device;

            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(device, &properties);


            uint64_t bytes = 0;
            VkPhysicalDeviceMemoryProperties memoryProperties;
            vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);
            for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; ++i) {
                if (memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                    bytes += memoryProperties.memoryHeaps[i].size;
                }
            }

            LOG_INFO("Selected GPU: {} | Vendor: {} | DeviceType: {} | Memory: {}",
                     properties.deviceName,
                     GetVendorName(properties.vendorID),
                     string_VkPhysicalDeviceType(properties.deviceType),
                     (bytes / (1024 * 1024)));
            break;
        }

        if (mPhysicalDevice == VK_NULL_HANDLE) {
            LOG_FATAL("Failed to find a suitable Vulkan physical device!");
        }
    }

    void VulkanDevice::CreateLogicalDevice() {

        float priority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

        for (uint32_t queueFamilyIndex : {mGraphicsQueueFamilyIndex, mPresentQueueFamilyIndex}) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
            queueCreateInfo.queueCount       = 1;
            queueCreateInfo.pQueuePriorities = &priority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures = {
            .samplerAnisotropy = VK_TRUE,
        };

        
        VkDeviceCreateInfo createInfo = {
            .sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
            .pQueueCreateInfos    = queueCreateInfos.data(),
            .enabledExtensionCount = static_cast<uint32_t>(mRequiredDeviceExtensions.size()),
            .ppEnabledExtensionNames = mRequiredDeviceExtensions.data(),
            .pEnabledFeatures    = &deviceFeatures,
        };

        if (vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mDevice) != VK_SUCCESS) {
            LOG_FATAL("Failed to create Vulkan logical device!");
        }

        vkGetDeviceQueue(mDevice, mGraphicsQueueFamilyIndex, 0, &mGraphicsQueue);
        vkGetDeviceQueue(mDevice, mPresentQueueFamilyIndex, 0, &mPresentQueue);

        LOG_INFO("Logical device created successfully.");
    }

    uint32_t VulkanDevice::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &memoryProperties);

        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
            if ((typeFilter & (1u << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        LOG_FATAL("Failed to find suitable memory type!");
        return UINT32_MAX;
    }

    VkFormat VulkanDevice::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(mPhysicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            }

            if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        LOG_FATAL("Failed to find supported format!");
        return VK_FORMAT_UNDEFINED;
    }

    VkFormat VulkanDevice::FindDepthFormat() const {
        return FindSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                                   VK_IMAGE_TILING_OPTIMAL,
                                   VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    bool VulkanDevice::HasStencilComponent(VkFormat format) const {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    void VulkanDevice::SetupDeviceQueues() {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyCount, nullptr);

        VkQueueFamilyProperties* queueFamilies = new VkQueueFamilyProperties[queueFamilyCount];
        vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyCount, queueFamilies);

        for (uint32_t i = 0; i < queueFamilyCount; ++i) {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                mGraphicsQueueFamilyIndex = i;
            }

            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(mPhysicalDevice, i, mSurface->GetHandle(), &presentSupport);

            if (presentSupport) {
                mPresentQueueFamilyIndex = i;
            }
        }

        delete[] queueFamilies;

        if (mGraphicsQueueFamilyIndex == UINT32_MAX || mPresentQueueFamilyIndex == UINT32_MAX) {
            LOG_FATAL("Failed to find suitable queue families for graphics and presentation!");
        }

        LOG_INFO("Graphics Queue Family Index: {} | Present Queue Family Index: {}", mGraphicsQueueFamilyIndex, mPresentQueueFamilyIndex);
    }



    // https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceProperties.html
    const char* VulkanDevice::GetVendorName(uint32_t vendorID) const {
        switch (vendorID) {
        case 0x1002:
            return "AMD";
        case 0x1010:
            return "ImgTec";
        case 0x10DE:
            return "NVIDIA";
        case 0x13B5:
            return "ARM";
        case 0x5143:
            return "Qualcomm";
        case 0x8086:
            return "Intel";
        default:
            return "Unknown Vendor";
        }
    }

} // namespace golias
