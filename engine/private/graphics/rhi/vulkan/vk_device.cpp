#include "graphics/rhi/vulkan/vk_device.h"

#include "graphics/rhi/vulkan/instance.h"
#include "graphics/rhi/vulkan/window_surface.h"


namespace golias {

    VulkanDevice::VulkanDevice(Ref<VulkanInstance> instance, Ref<VulkanWindowSurface> surface) : mInstance(instance), mSurface(surface) {
        PickPhysicalDevice();
        CreateLogicalDevice();
    }

    VulkanDevice::~VulkanDevice() {
        vkDestroyDevice(mDevice, nullptr);
    }

    void VulkanDevice::PickPhysicalDevice() {
        uint32_t deviceCount = 0;
        VkResult result      = vkEnumeratePhysicalDevices(mInstance->GetInstance(), &deviceCount, nullptr);
        VK_CHECK_RESULT(result);

        if (deviceCount == 0) {
            LOG_FATAL("Failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        result = vkEnumeratePhysicalDevices(mInstance->GetInstance(), &deviceCount, devices.data());
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
                result                  = vkGetPhysicalDeviceSurfaceSupportKHR(device, i, mSurface->GetSurface(), &presentSupport);
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


            uint64_t  bytes = 0;
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
                     GetDeviceTypeCString(properties.deviceType),
                     (bytes / (1024 * 1024)));
            break;
        }

        if (mPhysicalDevice == VK_NULL_HANDLE) {
            LOG_FATAL("Failed to find a suitable Vulkan physical device!");
        }
    }

    void VulkanDevice::CreateLogicalDevice() {
    }

    const char* VulkanDevice::GetDeviceTypeCString(VkPhysicalDeviceType type) const {
        switch (type) {
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            return "Other";
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            return "Integrated GPU";
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            return "Discrete GPU";
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            return "Virtual GPU";
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            return "CPU";
        default:
            return "Unknown";
        }
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
