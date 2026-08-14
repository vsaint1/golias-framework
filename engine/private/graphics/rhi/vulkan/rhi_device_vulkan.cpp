#include "graphics/rhi/vulkan/rhi_device_vulkan.h"

#include "core/window.h"
#include "graphics/rhi/vulkan/vk_common.h"
#include "graphics/rhi/vulkan/vk_surface.h"
#include <vulkan/vk_enum_string_helper.h>

namespace golias {

    constexpr std::array<const char*, 1> kDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    VKAPI_ATTR VkBool32 VKAPI_CALL RHIDeviceVulkan::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                                  VkDebugUtilsMessageTypeFlagsEXT,
                                                                  const VkDebugUtilsMessengerCallbackDataEXT* callback,
                                                                  void*) {
        if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            LOG_ERROR("Validation Layer: {}", callback->pMessage);
        } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            LOG_WARN("Validation Layer: {}", callback->pMessage);
        } else {
            LOG_DEBUG("Validation Layer: {}", callback->pMessage);
        }

        return VK_FALSE;
    }


    // Device lifecycle and presentation.


    RHIDeviceVulkan::RHIDeviceVulkan(Window* window, bool debug) : RHIDevice(window, debug) {

        if (!Initialize(window, debug)) {
            LOG_FATAL("Failed to initialize Vulkan device.");
        }

        LOG_INFO("Device initialized successfully.");
    }

    bool RHIDeviceVulkan::Initialize(Window* window, bool debug) {
        mWindow       = window;
        mDebugEnabled = debug;

        if (!CreateInstance(debug)) {
            return false;
        }

        if (debug) {
            VkDebugUtilsMessengerCreateInfoEXT info = {
                .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                .messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                 | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                .pfnUserCallback = &RHIDeviceVulkan::DebugCallback,
            };

            if (!VK_CHECK_RESULT(CreateDebugMessenger(mInstance, &info, &mDebugMessenger))) {
                LOG_WARN("Vulkan debug messenger is unavailable; continuing without it.");
            }
        }

        if (!VK_CHECK_RESULT(CreateVulkanSurface(*mWindow, mInstance, &mSurface))) {
            return false;
        }

        if (!CreateDevice()) {
            return false;
        }

        if (!CreateSwapchain()) {
            return false;
        }

        if (!CreateFrameContexts()) {
            return false;
        }

        mSetDebugUtilsObjectName =
            reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetDeviceProcAddr(mDevice, "vkSetDebugUtilsObjectNameEXT"));
        mCmdBeginDebugUtilsLabel =
            reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetDeviceProcAddr(mDevice, "vkCmdBeginDebugUtilsLabelEXT"));
        mCmdEndDebugUtilsLabel =
            reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetDeviceProcAddr(mDevice, "vkCmdEndDebugUtilsLabelEXT"));
        mCmdInsertDebugUtilsLabel =
            reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(vkGetDeviceProcAddr(mDevice, "vkCmdInsertDebugUtilsLabelEXT"));

        mCapabilities.supportsDebugMarkers = mCmdBeginDebugUtilsLabel && mCmdEndDebugUtilsLabel && mCmdInsertDebugUtilsLabel;

        LOG_INFO("Initialized Vulkan device '{}'", mInfo.deviceName);
        return true;
    }

    void RHIDeviceVulkan::SetVsyncEnabled(bool enabled) {
        if (mVsyncEnabled == enabled) {
            return;
        }

        mVsyncEnabled = enabled;
        if (mDevice && mSwapchain) {
            LOG_INFO("{} Vulkan presentation.", enabled ? "Enabling vsync" : "Disabling vsync");
            RecreateSwapchain();
        }
    }


    // Physical-device selection and logical-device creation.

    bool RHIDeviceVulkan::CheckValidationLayerSupport() const {
        uint32_t count = 0;
        if (!VK_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&count, nullptr))) {
            return false;
        }

        std::vector<VkLayerProperties> layers(count);
        vkEnumerateInstanceLayerProperties(&count, layers.data());

        for (const auto& layer : layers) {
            if (strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
                return true;
            }
        }

        return false;
    }

    bool RHIDeviceVulkan::CreateInstance(bool debug) {
        std::vector<const char*> extensions = GetVulkanInstanceExtensions();
        if (extensions.empty()) {
            return false;
        }

        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

        auto has_extension = [&](const char* name) {
            return std::any_of(availableExtensions.begin(), availableExtensions.end(), [&](const VkExtensionProperties& e) {
                return strcmp(e.extensionName, name) == 0;
            });
        };

        const bool hasDebugUtils = has_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        if (debug && hasDebugUtils) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        const bool validationAvailable = debug && CheckValidationLayerSupport();
        LOG_INFO("Creating Vulkan instance with {} required extensions{}.",
                 extensions.size(),
                 validationAvailable ? " and validation enabled" : "");

        const char* validationLayer = "VK_LAYER_KHRONOS_validation";
        VkApplicationInfo app       = {
            .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName   = "GOLIAS_ENGINE_APP",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName        = "GOLIAS_ENGINE",
            .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion         = VK_API_VERSION_1_3,
        };

        VkDebugUtilsMessengerCreateInfoEXT debugInfo = {
            .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = &RHIDeviceVulkan::DebugCallback,
        };

        VkInstanceCreateInfo info = {
            .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo        = &app,
            .enabledExtensionCount   = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data(),
        };

        if (validationAvailable) {
            info.enabledLayerCount   = 1;
            info.ppEnabledLayerNames = &validationLayer;
            if (hasDebugUtils) {
                info.pNext = &debugInfo;
            }
        }

        return VK_CHECK_RESULT(vkCreateInstance(&info, nullptr, &mInstance));
    }

    bool RHIDeviceVulkan::CheckDeviceExtensionSupport(VkPhysicalDevice device) const {
        uint32_t count = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);

        std::vector<VkExtensionProperties> available(count);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &count, available.data());

        for (const char* required : kDeviceExtensions) {
            bool found = false;
            for (const auto& extension : available) {
                if (strcmp(required, extension.extensionName) == 0) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                return false;
            }
        }

        return true;
    }

    RHIDeviceVulkan::QueueFamilyIndices RHIDeviceVulkan::FindQueueFamilies(VkPhysicalDevice device) const {
        QueueFamilyIndices result;

        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
        std::vector<VkQueueFamilyProperties> families(count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

        for (uint32_t i = 0; i < count; ++i) {
            if (families[i].queueCount == 0) {
                continue;
            }

            if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                result.graphics = i;
            }

            VkBool32 present = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, mSurface, &present);
            if (present) {
                result.present = i;
            }

            if (result.Complete()) {
                break;
            }
        }

        return result;
    }

    bool RHIDeviceVulkan::IsDeviceSuitable(VkPhysicalDevice device) const {
        if (!CheckDeviceExtensionSupport(device)) {
            return false;
        }

        const QueueFamilyIndices families = FindQueueFamilies(device);
        if (!families.Complete()) {
            return false;
        }

        VkPhysicalDeviceFeatures features = {
            .robustBufferAccess = VK_FALSE,
        };

        vkGetPhysicalDeviceFeatures(device, &features);

        VkPhysicalDeviceDynamicRenderingFeatures dynamicRendering = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
        };

        VkPhysicalDeviceFeatures2 features2 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = &dynamicRendering,
        };

        vkGetPhysicalDeviceFeatures2(device, &features2);
        if (!dynamicRendering.dynamicRendering) {
            return false;
        }

        VkPhysicalDeviceProperties properties = {
            .apiVersion = 0,
        };

        vkGetPhysicalDeviceProperties(device, &properties);

        if (!features.samplerAnisotropy) {
            return false;
        }

        uint32_t formatCount  = 0;
        uint32_t presentCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, mSurface, &formatCount, nullptr);

        vkGetPhysicalDeviceSurfacePresentModesKHR(device, mSurface, &presentCount, nullptr);

        return formatCount > 0 && presentCount > 0 && VK_VERSION_MAJOR(properties.apiVersion) >= 1
            && VK_VERSION_MINOR(properties.apiVersion) >= 3;
    }

    bool RHIDeviceVulkan::CreateDevice() {
        uint32_t count = 0;
        vkEnumeratePhysicalDevices(mInstance, &count, nullptr);
        if (count == 0) {
            LOG_ERROR("No physical devices were found.");
            return false;
        }

        std::vector<VkPhysicalDevice> devices(count);
        vkEnumeratePhysicalDevices(mInstance, &count, devices.data());

        for (VkPhysicalDevice device : devices) {
            if (IsDeviceSuitable(device)) {
                mPhysicalDevice = device;
                break;
            }
        }

        if (!mPhysicalDevice) {
            LOG_ERROR("No suitable physical device was found.");
            return false;
        }

        mQueueFamilies = FindQueueFamilies(mPhysicalDevice);

        VkPhysicalDeviceProperties properties = {
            .apiVersion = 0,
        };

        vkGetPhysicalDeviceProperties(mPhysicalDevice, &properties);

        VkPhysicalDeviceFeatures supported = {
            .robustBufferAccess = VK_FALSE,
        };

        vkGetPhysicalDeviceFeatures(mPhysicalDevice, &supported);

        std::set<uint32_t> uniqueFamilies = {mQueueFamilies.graphics, mQueueFamilies.present};

        float priority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> queues;
        for (uint32_t family : uniqueFamilies) {
            VkDeviceQueueCreateInfo queue = {
                .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = family,
                .queueCount       = 1,
                .pQueuePriorities = &priority,
            };

            queues.push_back(queue);
        }

        VkPhysicalDeviceDynamicRenderingFeatures dynamicRendering = {
            .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
            .dynamicRendering = VK_TRUE,
        };

        VkPhysicalDeviceFeatures enabledFeatures = supported;
        enabledFeatures.samplerAnisotropy        = VK_TRUE;
        enabledFeatures.fillModeNonSolid         = supported.fillModeNonSolid;

        VkPhysicalDeviceFeatures2 features2 = {
            .sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext    = &dynamicRendering,
            .features = enabledFeatures,
        };

        VkDeviceCreateInfo info = {
            .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext                   = &features2,
            .queueCreateInfoCount    = static_cast<uint32_t>(queues.size()),
            .pQueueCreateInfos       = queues.data(),
            .enabledExtensionCount   = static_cast<uint32_t>(kDeviceExtensions.size()),
            .ppEnabledExtensionNames = kDeviceExtensions.data(),
        };

        if (!VK_CHECK_RESULT(vkCreateDevice(mPhysicalDevice, &info, nullptr, &mDevice))) {
            return false;
        }

        vkGetDeviceQueue(mDevice, mQueueFamilies.graphics, 0, &mGraphicsQueue);
        vkGetDeviceQueue(mDevice, mQueueFamilies.present, 0, &mPresentQueue);

        mInfo.deviceName = properties.deviceName;
        mInfo.driverName = "Vulkan";

        char apiVersionStr[256] = "1.3.0";

        sprintf(apiVersionStr,
                "%d.%d.%d",
                VK_VERSION_MAJOR(properties.apiVersion),
                VK_VERSION_MINOR(properties.apiVersion),
                VK_VERSION_PATCH(properties.apiVersion));

        mInfo.apiVersion = apiVersionStr;

        mInfo.backend  = RHIBackend::Vulkan;
        mInfo.vendorId = properties.vendorID;
        mInfo.deviceId = properties.deviceID;

        mCapabilities.supportsCompute        = (properties.limits.maxComputeWorkGroupInvocations > 0);
        mCapabilities.supportsStorageBuffers = true;
        mCapabilities.supportsMultisampling  = true;
        mCapabilities.supportsAnisotropy     = supported.samplerAnisotropy;
        mCapabilities.supportsWireframe      = supported.fillModeNonSolid;

        const VkSampleCountFlags counts = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;

        if (counts & VK_SAMPLE_COUNT_8_BIT) {
            mCapabilities.maxSampleCount = SampleCount::Count8;
        } else if (counts & VK_SAMPLE_COUNT_4_BIT) {
            mCapabilities.maxSampleCount = SampleCount::Count4;
        } else if (counts & VK_SAMPLE_COUNT_2_BIT) {
            mCapabilities.maxSampleCount = SampleCount::Count2;
        } else {
            mCapabilities.maxSampleCount = SampleCount::Count1;
        }

        mCapabilities.maxTextureSize  = properties.limits.maxImageDimension2D;
        mCapabilities.maxColorTargets = properties.limits.maxColorAttachments;

        LOG_INFO("Selected Vulkan adapter '{}' (API {})", mInfo.deviceName, mInfo.apiVersion);
        return true;
    }


    // Swapchain and frame synchronization.

    bool RHIDeviceVulkan::CreateSwapchain() {

        VkSurfaceCapabilitiesKHR capabilities = {
            .minImageCount = 0,
        };

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mPhysicalDevice, mSurface, &capabilities);

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDevice, mSurface, &formatCount, nullptr);
        if (formatCount == 0) {
            LOG_ERROR("Vulkan surface does not expose any swapchain formats.");
            return false;
        }

        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDevice, mSurface, &formatCount, formats.data());

        VkSurfaceFormatKHR selected = formats[0];
        for (const auto& format : formats) {
            if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                selected = format;
                break;
            }
        }

        if (mSwapchain != VK_NULL_HANDLE) {
            for (const auto& format : formats) {
                if (format.format == mSwapchainFormat && format.colorSpace == mSwapchainColorSpace) {
                    selected = format;
                    break;
                }
            }
        }

        mSwapchainFormat     = selected.format;
        mSwapchainColorSpace = selected.colorSpace;

        int width  = 0;
        int height = 0;
        mWindow->GetFramebufferSize(&width, &height);

        if (width <= 0 || height <= 0) {
            LOG_CRITICAL("Swapchain creation failed: invalid framebuffer size ({}x{})", width, height);
            return false;
        }

        VkExtent2D extent;
        if (capabilities.currentExtent.width != UINT32_MAX) {
            extent = capabilities.currentExtent;
        } else {
            extent.width = std::clamp(static_cast<uint32_t>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            extent.height =
                std::clamp(static_cast<uint32_t>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        }

        mSwapchainExtent = extent;

        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount != 0) {
            imageCount = std::min(imageCount, capabilities.maxImageCount);
        }

        VkSwapchainCreateInfoKHR info = {
            .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface          = mSurface,
            .minImageCount    = imageCount,
            .imageFormat      = mSwapchainFormat,
            .imageColorSpace  = mSwapchainColorSpace,
            .imageExtent      = mSwapchainExtent,
            .imageArrayLayers = 1,
            .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .preTransform     = capabilities.currentTransform,
            .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode      = VK_PRESENT_MODE_FIFO_KHR,
            .clipped          = VK_TRUE,
        };

        if (!mVsyncEnabled) {

            uint32_t presentCount = 0;
            vkGetPhysicalDeviceSurfacePresentModesKHR(mPhysicalDevice, mSurface, &presentCount, nullptr);

            std::vector<VkPresentModeKHR> modes(presentCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(mPhysicalDevice, mSurface, &presentCount, modes.data());

            bool mailboxAvailable = false;
            for (VkPresentModeKHR mode : modes) {
                if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                    info.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                    break;
                }

                if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                    mailboxAvailable = true;
                }
            }

            if (info.presentMode == VK_PRESENT_MODE_FIFO_KHR && mailboxAvailable) {
                info.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            }
        }

        const uint32_t queueFamilyIndices[] = {mQueueFamilies.graphics, mQueueFamilies.present};

        if (mQueueFamilies.graphics != mQueueFamilies.present) {
            info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
            info.queueFamilyIndexCount = 2;
            info.pQueueFamilyIndices   = queueFamilyIndices;
        } else {
            info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        if (!VK_CHECK_RESULT(vkCreateSwapchainKHR(mDevice, &info, nullptr, &mSwapchain))) {
            return false;
        }

        LOG_INFO("Created Vulkan swapchain {}x{} with {} images ({}).",
                 mSwapchainExtent.width,
                 mSwapchainExtent.height,
                 imageCount,
                 string_VkPresentModeKHR(info.presentMode));

        uint32_t actualCount = 0;
        vkGetSwapchainImagesKHR(mDevice, mSwapchain, &actualCount, nullptr);

        mSwapchainImages.resize(actualCount);
        vkGetSwapchainImagesKHR(mDevice, mSwapchain, &actualCount, mSwapchainImages.data());

        mSwapchainImageViews.resize(actualCount);
        mSwapchainLayouts.assign(actualCount, VK_IMAGE_LAYOUT_UNDEFINED);
        mImagesInFlight.assign(actualCount, VK_NULL_HANDLE);

        for (uint32_t i = 0; i < actualCount; ++i) {
            VkImageViewCreateInfo view = {
                .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image    = mSwapchainImages[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format   = mSwapchainFormat,
                .subresourceRange =
                    {
                                       .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                                       .baseMipLevel   = 0,
                                       .levelCount     = 1,
                                       .baseArrayLayer = 0,
                                       .layerCount     = 1,
                                       },
            };

            if (!VK_CHECK_RESULT(vkCreateImageView(mDevice, &view, nullptr, &mSwapchainImageViews[i]))) {
                LOG_CRITICAL("Failed to create image view for swapchain image {}", i);
                return false;
            }

            SetObjectName(
                VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64_t>(mSwapchainImages[i]), ("SwapchainImage" + std::to_string(i)).c_str());
        }

        LOG_DEBUG("Created {} swapchain image views.", actualCount);
        return true;
    }

    bool RHIDeviceVulkan::CreateFrameContexts() {

        VkPhysicalDeviceProperties properties = {
            .apiVersion = 0,
        };

        vkGetPhysicalDeviceProperties(mPhysicalDevice, &properties);

        for (uint32_t i = 0; i < MaxFramesInFlight; ++i) {
            VkCommandPoolCreateInfo pool = {
                .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                .queueFamilyIndex = mQueueFamilies.graphics,
            };

            if (!VK_CHECK_RESULT(vkCreateCommandPool(mDevice, &pool, nullptr, &mFrames[i].commandPool))) {
                return false;
            }

            VkCommandBufferAllocateInfo commandAllocation = {
                .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool        = mFrames[i].commandPool,
                .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
            };

            if (!VK_CHECK_RESULT(vkAllocateCommandBuffers(mDevice, &commandAllocation, &mFrames[i].commandBuffer))) {
                return false;
            }

            VkSemaphoreCreateInfo semaphore = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            };

            if (!VK_CHECK_RESULT(vkCreateSemaphore(mDevice, &semaphore, nullptr, &mFrames[i].imageAvailable))) {
                return false;
            }

            if (!VK_CHECK_RESULT(vkCreateSemaphore(mDevice, &semaphore, nullptr, &mFrames[i].renderFinished))) {
                return false;
            }

            VkFenceCreateInfo fence = {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT,
            };

            if (!VK_CHECK_RESULT(vkCreateFence(mDevice, &fence, nullptr, &mFrames[i].fence))) {
                return false;
            }

            VkDescriptorPoolSize sizes[] = {
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         4096},
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4096},
                {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          4096},
                {VK_DESCRIPTOR_TYPE_SAMPLER,                4096},
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         4096},
                {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          4096},
            };

            VkDescriptorPoolCreateInfo descriptorPool = {
                .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
                .maxSets       = 4096,
                .poolSizeCount = static_cast<uint32_t>(std::size(sizes)),
                .pPoolSizes    = sizes,
            };

            if (!VK_CHECK_RESULT(vkCreateDescriptorPool(mDevice, &descriptorPool, nullptr, &mFrames[i].descriptorPool))) {
                return false;
            }
        }

        LOG_DEBUG("Created {} Vulkan frame contexts.", MaxFramesInFlight);
        return true;
    }

    void RHIDeviceVulkan::DestroyFrameContexts() {
        for (auto& frame : mFrames) {
            if (frame.descriptorPool) {
                vkDestroyDescriptorPool(mDevice, frame.descriptorPool, nullptr);
            }

            if (frame.fence) {
                vkDestroyFence(mDevice, frame.fence, nullptr);
            }

            if (frame.imageAvailable) {
                vkDestroySemaphore(mDevice, frame.imageAvailable, nullptr);
            }

            if (frame.renderFinished) {
                vkDestroySemaphore(mDevice, frame.renderFinished, nullptr);
            }

            if (frame.commandPool) {
                vkDestroyCommandPool(mDevice, frame.commandPool, nullptr);
            }

            frame = {};
        }
    }

    void RHIDeviceVulkan::DestroySwapchain() {
        for (VkImageView view : mSwapchainImageViews) {
            if (view) {
                vkDestroyImageView(mDevice, view, nullptr);
            }
        }

        mSwapchainImageViews.clear();
        mSwapchainImages.clear();
        mSwapchainLayouts.clear();
        mImagesInFlight.clear();

        if (mSwapchain) {
            vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);
            mSwapchain = VK_NULL_HANDLE;
        }
    }

    bool RHIDeviceVulkan::RecreateSwapchain() {
        int width  = 0;
        int height = 0;
        mWindow->GetFramebufferSize(&width, &height);

        while (width == 0 || height == 0) {
            mWindow->WaitForEvents();
            mWindow->GetFramebufferSize(&width, &height);
        }

        vkDeviceWaitIdle(mDevice);
        LOG_INFO("Recreating Vulkan swapchain for framebuffer size {}x{}.", width, height);

        DestroySwapchain();
        const bool recreated = CreateSwapchain();
        if (!recreated) {
            LOG_ERROR("Failed to recreate Vulkan swapchain.");
        }

        return recreated;
    }


    // Resource creation and format conversion.

    uint32_t RHIDeviceVulkan::FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties) const {

        VkPhysicalDeviceMemoryProperties memoryProperties = {
            .memoryTypeCount = 0,
        };

        vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &memoryProperties);

        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
            if ((typeBits & (1u << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        return UINT32_MAX;
    }

    bool RHIDeviceVulkan::CreateImage(const TextureDesc& desc, TextureState& state) {
        state.rhiFormat   = desc.format;
        state.format      = ToVkFormat(desc.format, mSwapchainFormat);
        state.aspect      = AspectForFormat(desc.format);
        state.width       = desc.width;
        state.height      = desc.height;
        state.layers      = desc.type == TextureType::TextureCube ? 6 : std::max(1u, desc.depthOrLayers);
        state.mipLevels   = std::max(1u, desc.mipLevels);
        state.type        = desc.type;
        state.usage       = desc.usage;
        state.sampleCount = desc.sampleCount;

        if (state.format == VK_FORMAT_UNDEFINED) {
            return false;
        }

        VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        if (state.mipLevels > 1) {
            usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }

        if (HasFlag(desc.usage, TextureUsage::Sampler)) {
            usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }

        if (HasFlag(desc.usage, TextureUsage::ColorTarget)) {
            usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }

        if (HasFlag(desc.usage, TextureUsage::DepthTarget)) {
            usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }

        if (HasFlag(desc.usage, TextureUsage::StorageRead) || HasFlag(desc.usage, TextureUsage::StorageWrite)
            || HasFlag(desc.usage, TextureUsage::ComputeWrite)) {
            usage |= VK_IMAGE_USAGE_STORAGE_BIT;
        }

        VkImageCreateInfo info = {
            .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType     = VK_IMAGE_TYPE_2D,
            .format        = state.format,
            .extent        = {state.width, state.height, 1},
            .mipLevels     = state.mipLevels,
            .arrayLayers   = state.layers,
            .samples       = ToVkSampleCount(desc.sampleCount),
            .tiling        = VK_IMAGE_TILING_OPTIMAL,
            .usage         = usage,
            .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        if (desc.type == TextureType::TextureCube) {
            info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }

        if (!VK_CHECK_RESULT(vkCreateImage(mDevice, &info, nullptr, &state.image))) {
            return false;
        }

        VkMemoryRequirements requirements = {
            .size = 0,
        };
        vkGetImageMemoryRequirements(mDevice, state.image, &requirements);

        const uint32_t memoryType = FindMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (memoryType == UINT32_MAX) {
            vkDestroyImage(mDevice, state.image, nullptr);
            state.image = VK_NULL_HANDLE;
            return false;
        }

        VkMemoryAllocateInfo allocation = {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize  = requirements.size,
            .memoryTypeIndex = memoryType,
        };

        if (!VK_CHECK_RESULT(vkAllocateMemory(mDevice, &allocation, nullptr, &state.memory))) {
            vkDestroyImage(mDevice, state.image, nullptr);
            state.image = VK_NULL_HANDLE;
            return false;
        }

        if (!VK_CHECK_RESULT(vkBindImageMemory(mDevice, state.image, state.memory, 0))) {
            return false;
        }

        if (!CreateImageView(state)) {
            return false;
        }

        state.layout = VK_IMAGE_LAYOUT_UNDEFINED;
        return true;
    }

    bool RHIDeviceVulkan::CreateImageView(TextureState& state) {
        const VkImageViewType viewType = state.type == TextureType::TextureCube    ? VK_IMAGE_VIEW_TYPE_CUBE
                                       : state.type == TextureType::Texture2DArray ? VK_IMAGE_VIEW_TYPE_2D_ARRAY
                                                                                   : VK_IMAGE_VIEW_TYPE_2D;
        VkImageViewCreateInfo view     = {
            .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image    = state.image,
            .viewType = viewType,
            .format   = state.format,
            .subresourceRange =
                {
                                   .aspectMask     = state.aspect,
                                   .baseMipLevel   = 0,
                                   .levelCount     = state.mipLevels,
                                   .baseArrayLayer = 0,
                                   .layerCount     = state.layers,
                                   },
        };

        return VK_CHECK_RESULT(vkCreateImageView(mDevice, &view, nullptr, &state.view));
    }

    bool RHIDeviceVulkan::CreateBufferInternal(const BufferDesc& desc, BufferState& state) {

        VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        if (HasFlag(desc.usage, BufferUsage::Vertex)) {
            usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        }

        if (HasFlag(desc.usage, BufferUsage::Index)) {
            usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        }

        if (HasFlag(desc.usage, BufferUsage::Uniform)) {
            usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        }

        if (HasFlag(desc.usage, BufferUsage::Storage)) {
            usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        }

        if (HasFlag(desc.usage, BufferUsage::Indirect)) {
            usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
        }

        const bool hostVisible = HasFlag(desc.usage, BufferUsage::Uniform);

        if (hostVisible) {
            usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        }

        VkBufferCreateInfo info = {
            .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size        = std::max<uint32_t>(1, desc.size),
            .usage       = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        if (!VK_CHECK_RESULT(vkCreateBuffer(mDevice, &info, nullptr, &state.buffer))) {
            return false;
        }

        VkMemoryRequirements requirements = {
            .size = 0,
        };
        vkGetBufferMemoryRequirements(mDevice, state.buffer, &requirements);

        const VkMemoryPropertyFlags properties =
            hostVisible ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        const uint32_t memoryType = FindMemoryType(requirements.memoryTypeBits, properties);

        if (memoryType == UINT32_MAX) {
            vkDestroyBuffer(mDevice, state.buffer, nullptr);
            state.buffer = VK_NULL_HANDLE;
            return false;
        }

        VkMemoryAllocateInfo allocation = {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize  = requirements.size,
            .memoryTypeIndex = memoryType,
        };

        if (!VK_CHECK_RESULT(vkAllocateMemory(mDevice, &allocation, nullptr, &state.memory))) {
            vkDestroyBuffer(mDevice, state.buffer, nullptr);
            state.buffer = VK_NULL_HANDLE;
            return false;
        }

        if (!VK_CHECK_RESULT(vkBindBufferMemory(mDevice, state.buffer, state.memory, 0))) {
            return false;
        }

        state.size             = desc.size;
        state.memoryProperties = properties;
        return true;
    }

    bool RHIDeviceVulkan::CreateStagingBuffer(VkDeviceSize size, BufferState& state) {
        BufferDesc desc{};
        desc.usage = BufferUsage::Vertex;
        desc.size  = static_cast<uint32_t>(std::min<VkDeviceSize>(size, std::numeric_limits<uint32_t>::max()));

        VkBufferCreateInfo info = {
            .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size        = size,
            .usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        if (!VK_CHECK_RESULT(vkCreateBuffer(mDevice, &info, nullptr, &state.buffer))) {
            return false;
        }

        VkMemoryRequirements requirements = {
            .size = 0,
        };
        vkGetBufferMemoryRequirements(mDevice, state.buffer, &requirements);

        const uint32_t memoryType =
            FindMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (memoryType == UINT32_MAX) {
            return false;
        }

        VkMemoryAllocateInfo allocation = {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize  = requirements.size,
            .memoryTypeIndex = memoryType,
        };

        if (!VK_CHECK_RESULT(vkAllocateMemory(mDevice, &allocation, nullptr, &state.memory))) {
            return false;
        }

        if (!VK_CHECK_RESULT(vkBindBufferMemory(mDevice, state.buffer, state.memory, 0))) {
            return false;
        }

        state.size             = size;
        state.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        return true;
    }

    void RHIDeviceVulkan::DestroyBufferState(BufferState& state) {
        if (state.buffer) {
            vkDestroyBuffer(mDevice, state.buffer, nullptr);
        }
        if (state.memory) {
            vkFreeMemory(mDevice, state.memory, nullptr);
        }
        state = {};
    }

    void RHIDeviceVulkan::DestroyTextureState(TextureState& state) {
        if (state.view) {
            vkDestroyImageView(mDevice, state.view, nullptr);
        }
        if (state.image) {
            vkDestroyImage(mDevice, state.image, nullptr);
        }
        if (state.memory) {
            vkFreeMemory(mDevice, state.memory, nullptr);
        }
        state = {};
    }

    TextureHandle RHIDeviceVulkan::CreateTexture(const TextureDesc& desc) {
        if (desc.format == TextureFormat::Swapchain) {
            return TextureHandle{SwapchainTextureId};
        }

        TextureState state{};
        if (!CreateImage(desc, state)) {
            LOG_ERROR("Failed to create Vulkan texture ({}x{}, format {})", desc.width, desc.height, static_cast<int>(desc.format));
            return {};
        }

        const uint64_t id = NextId();
        mTextures.emplace(id, state);
        return TextureHandle{id};
    }

    void RHIDeviceVulkan::DestroyTexture(TextureHandle texture) {
        if (!texture || texture.id == SwapchainTextureId) {
            return;
        }

        auto it = mTextures.find(texture.id);
        if (it == mTextures.end()) {
            return;
        }

        vkDeviceWaitIdle(mDevice);
        DestroyTextureState(it->second);
        mTextures.erase(it);
    }

    BufferHandle RHIDeviceVulkan::CreateBuffer(const BufferDesc& desc) {
        if (desc.size == 0) {
            return {};
        }

        BufferState state{};
        if (!CreateBufferInternal(desc, state)) {
            LOG_ERROR("Failed to create Vulkan buffer of {} bytes.", desc.size);
            return {};
        }

        const uint64_t id = NextId();
        mBuffers.emplace(id, state);
        return BufferHandle{id};
    }

    BufferHandle RHIDeviceVulkan::AcquireTransientUniformBuffer(uint32_t size) {
        auto& available = mReusableUniformBuffers[size];
        if (!available.empty()) {
            const BufferHandle buffer = available.back();
            available.pop_back();
            return buffer;
        }

        return CreateBuffer({.usage = BufferUsage::Uniform, .size = size});
    }

    void RHIDeviceVulkan::DestroyBuffer(BufferHandle buffer) {
        if (!buffer) {
            return;
        }

        auto it = mBuffers.find(buffer.id);
        if (it == mBuffers.end()) {
            return;
        }

        vkDeviceWaitIdle(mDevice);
        DestroyBufferState(it->second);
        mBuffers.erase(it);
    }

    SamplerHandle RHIDeviceVulkan::CreateSampler(const SamplerDesc& desc) {
        VkPhysicalDeviceProperties properties = {
            .apiVersion = 0,
        };
        vkGetPhysicalDeviceProperties(mPhysicalDevice, &properties);

        const bool anisotropy    = desc.enableAnisotropy && mCapabilities.supportsAnisotropy;
        VkSamplerCreateInfo info = {
            .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter               = ToVkFilter(desc.magFilter),
            .minFilter               = ToVkFilter(desc.minFilter),
            .mipmapMode              = ToVkMipmapMode(desc.mipmapMode),
            .addressModeU            = ToVkAddressMode(desc.addressU),
            .addressModeV            = ToVkAddressMode(desc.addressV),
            .addressModeW            = ToVkAddressMode(desc.addressW),
            .mipLodBias              = 0.0f,
            .anisotropyEnable        = anisotropy ? VK_TRUE : VK_FALSE,
            .maxAnisotropy           = anisotropy ? std::clamp(desc.maxAnisotropy, 1.0f, properties.limits.maxSamplerAnisotropy) : 1.0f,
            .minLod                  = 0.0f,
            .maxLod                  = VK_LOD_CLAMP_NONE,
            .unnormalizedCoordinates = VK_FALSE,
        };

        VkSampler sampler = VK_NULL_HANDLE;
        if (!VK_CHECK_RESULT(vkCreateSampler(mDevice, &info, nullptr, &sampler))) {
            return {};
        }

        const uint64_t id = NextId();
        mSamplers.emplace(id, SamplerState{sampler});
        return SamplerHandle{id};
    }

    void RHIDeviceVulkan::DestroySampler(SamplerHandle sampler) {
        if (!sampler) {
            return;
        }

        auto it = mSamplers.find(sampler.id);
        if (it == mSamplers.end()) {
            return;
        }

        vkDeviceWaitIdle(mDevice);
        vkDestroySampler(mDevice, it->second.sampler, nullptr);
        mSamplers.erase(it);
    }

    ShaderHandle RHIDeviceVulkan::CreateShader(const ShaderDesc& desc) {
        VkShaderModule module = CreateShaderModule(mDevice, desc.code, desc.size);
        if (!module) {
            LOG_ERROR("Failed to create Vulkan shader module.");
            return {};
        }

        ShaderState state{};
        state.module             = module;
        state.stage              = desc.stage;
        state.entryPoint         = desc.entrypoint ? desc.entrypoint : "main";
        state.numSamplers        = desc.numSamplers;
        state.numUniformBuffers  = desc.numUniformBuffers;
        state.numStorageTextures = desc.numStorageTextures;
        state.numStorageBuffers  = desc.numStorageBuffers;

        const uint64_t id = NextId();
        mShaders.emplace(id, std::move(state));
        return ShaderHandle{id};
    }

    void RHIDeviceVulkan::DestroyShader(ShaderHandle shader) {
        if (!shader) {
            return;
        }

        auto it = mShaders.find(shader.id);
        if (it == mShaders.end()) {
            return;
        }

        vkDeviceWaitIdle(mDevice);
        vkDestroyShaderModule(mDevice, it->second.module, nullptr);
        mShaders.erase(it);
    }

    VkDescriptorSetLayout RHIDeviceVulkan::CreateDescriptorSetLayout(uint32_t numUniformBuffers,
                                                                     uint32_t numSamplers,
                                                                     uint32_t numStorageTextures,
                                                                     uint32_t numStorageBuffers,
                                                                     VkShaderStageFlags stages) {

        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(numUniformBuffers + numSamplers * 2 + numStorageTextures + numStorageBuffers);

        uint32_t binding = 0;

        for (uint32_t i = 0; i < numUniformBuffers; ++i, ++binding) {
            bindings.push_back({.binding            = binding,
                                .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                .descriptorCount    = 1,
                                .stageFlags         = stages,
                                .pImmutableSamplers = nullptr});
        }

        for (uint32_t i = 0; i < numSamplers; ++i) {
            bindings.push_back({.binding            = binding,
                                .descriptorType     = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                                .descriptorCount    = 1,
                                .stageFlags         = stages,
                                .pImmutableSamplers = nullptr});
            ++binding;
            bindings.push_back({.binding            = binding,
                                .descriptorType     = VK_DESCRIPTOR_TYPE_SAMPLER,
                                .descriptorCount    = 1,
                                .stageFlags         = stages,
                                .pImmutableSamplers = nullptr});
            ++binding;
        }

        for (uint32_t i = 0; i < numStorageTextures; ++i, ++binding) {
            bindings.push_back({.binding            = binding,
                                .descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                                .descriptorCount    = 1,
                                .stageFlags         = stages,
                                .pImmutableSamplers = nullptr});
        }

        for (uint32_t i = 0; i < numStorageBuffers; ++i, ++binding) {
            bindings.push_back({.binding            = binding,
                                .descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                .descriptorCount    = 1,
                                .stageFlags         = stages,
                                .pImmutableSamplers = nullptr});
        }

        VkDescriptorSetLayoutCreateInfo info = {
            .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings    = bindings.data(),
        };

        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        if (!VK_CHECK_RESULT(vkCreateDescriptorSetLayout(mDevice, &info, nullptr, &layout))) {
            return VK_NULL_HANDLE;
        }

        return layout;
    }

    bool RHIDeviceVulkan::CreatePipelineLayout(std::array<VkDescriptorSetLayout, 3>& layouts, VkPipelineLayout& layout) {

        for (uint32_t i = 0; i < 3; ++i) {
            if (!layouts[i]) {
                layouts[i] = CreateDescriptorSetLayout(0,
                                                       0,
                                                       0,
                                                       0,
                                                       i == 0   ? (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
                                                       : i == 1 ? VK_SHADER_STAGE_FRAGMENT_BIT
                                                                : VK_SHADER_STAGE_COMPUTE_BIT);
            }

            if (!layouts[i]) {
                return false;
            }
        }

        VkPipelineLayoutCreateInfo info = {
            .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 3,
            .pSetLayouts    = layouts.data(),
        };

        return VK_CHECK_RESULT(vkCreatePipelineLayout(mDevice, &info, nullptr, &layout));
    }

    GraphicsPipelineHandle RHIDeviceVulkan::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) {

        auto vertex   = mShaders.find(desc.vertexShader.id);
        auto fragment = mShaders.find(desc.fragmentShader.id);

        if (vertex == mShaders.end() || fragment == mShaders.end()) {
            return {};
        }

        if (vertex->second.stage != ShaderStage::Vertex || fragment->second.stage != ShaderStage::Fragment) {
            return {};
        }

        PipelineState state{};

        state.setLayouts[0] = CreateDescriptorSetLayout(vertex->second.numUniformBuffers,
                                                        vertex->second.numSamplers,
                                                        vertex->second.numStorageTextures,
                                                        vertex->second.numStorageBuffers,
                                                        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

        state.setLayouts[1] = CreateDescriptorSetLayout(fragment->second.numUniformBuffers,
                                                        fragment->second.numSamplers,
                                                        fragment->second.numStorageTextures,
                                                        fragment->second.numStorageBuffers,
                                                        VK_SHADER_STAGE_FRAGMENT_BIT);

        if (!CreatePipelineLayout(state.setLayouts, state.layout)) {
            return {};
        }

        std::array<VkPipelineShaderStageCreateInfo, 2> stages{};

        stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                     nullptr,
                     0,
                     VK_SHADER_STAGE_VERTEX_BIT,
                     vertex->second.module,
                     vertex->second.entryPoint.c_str(),
                     nullptr};

        stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                     nullptr,
                     0,
                     VK_SHADER_STAGE_FRAGMENT_BIT,
                     fragment->second.module,
                     fragment->second.entryPoint.c_str(),
                     nullptr};

        std::vector<VkVertexInputBindingDescription> bindings;
        std::vector<VkVertexInputAttributeDescription> attributes;

        for (uint32_t i = 0; i < desc.vertexInput.numBuffers; ++i) {
            const auto& input = desc.vertexInput.bufferDescs[i];
            bindings.push_back({
                .binding   = input.slot,
                .stride    = input.stride,
                .inputRate = input.instanced ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX,
            });
        }

        auto format = [](VertexElementFormat f) {
            switch (f) {
            case VertexElementFormat::Float:
                return VK_FORMAT_R32_SFLOAT;
            case VertexElementFormat::Float2:
                return VK_FORMAT_R32G32_SFLOAT;
            case VertexElementFormat::Float3:
                return VK_FORMAT_R32G32B32_SFLOAT;
            case VertexElementFormat::Float4:
                return VK_FORMAT_R32G32B32A32_SFLOAT;
            case VertexElementFormat::Int:
                return VK_FORMAT_R32_SINT;
            case VertexElementFormat::Int2:
                return VK_FORMAT_R32G32_SINT;
            case VertexElementFormat::Int3:
                return VK_FORMAT_R32G32B32_SINT;
            case VertexElementFormat::Int4:
                return VK_FORMAT_R32G32B32A32_SINT;
            case VertexElementFormat::UByte4_Norm:
                return VK_FORMAT_R8G8B8A8_UNORM;
            default:
                return VK_FORMAT_R32G32_SFLOAT;
            }
        };

        for (uint32_t i = 0; i < desc.vertexInput.numAttributes; ++i) {
            const auto& attribute = desc.vertexInput.attributes[i];
            attributes.push_back({
                .location = attribute.location,
                .binding  = attribute.bufferSlot,
                .format   = format(attribute.format),
                .offset   = attribute.offset,
            });
        }

        VkPipelineVertexInputStateCreateInfo vertexInput = {
            .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = static_cast<uint32_t>(bindings.size()),
            .pVertexBindingDescriptions      = bindings.data(),
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size()),
            .pVertexAttributeDescriptions    = attributes.data(),
        };

        VkPipelineInputAssemblyStateCreateInfo assembly = {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology               = ToVkTopology(desc.primitiveType),
            .primitiveRestartEnable = VK_FALSE,
        };

        const bool depthBias                          = desc.rasterizerState.enableDepthBias;
        VkPipelineRasterizationStateCreateInfo raster = {
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable        = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode             = desc.rasterizerState.fillMode == FillMode::Line ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL,
            .cullMode                = ToVkCullMode(desc.rasterizerState.cullMode),
            .frontFace               = ToVkFrontFace(desc.rasterizerState.frontFace),
            .depthBiasEnable         = depthBias ? VK_TRUE : VK_FALSE,
            .depthBiasConstantFactor = depthBias ? desc.rasterizerState.depthBiasConstantFactor : 0.0f,
            .depthBiasClamp          = depthBias ? desc.rasterizerState.depthBiasClamp : 0.0f,
            .depthBiasSlopeFactor    = depthBias ? desc.rasterizerState.depthBiasSlopeFactor : 0.0f,
            .lineWidth               = 1.0f,
        };

        VkPipelineMultisampleStateCreateInfo multisample = {
            .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = ToVkSampleCount(desc.sampleCount),
        };

        std::vector<VkPipelineColorBlendAttachmentState> blendAttachments;
        blendAttachments.reserve(desc.targetInfo.numColorTargets);

        for (uint32_t i = 0; i < desc.targetInfo.numColorTargets; ++i) {
            const auto& target = desc.targetInfo.colorTargets[i];
            const auto& blend  = target.blendState;

            VkPipelineColorBlendAttachmentState attachment = {
                .blendEnable         = blend.enableBlend ? VK_TRUE : VK_FALSE,
                .srcColorBlendFactor = ToVkBlendFactor(blend.srcColorFactor),
                .dstColorBlendFactor = ToVkBlendFactor(blend.dstColorFactor),
                .colorBlendOp        = ToVkBlendOp(blend.colorOp),
                .srcAlphaBlendFactor = ToVkBlendFactor(blend.srcAlphaFactor),
                .dstAlphaBlendFactor = ToVkBlendFactor(blend.dstAlphaFactor),
                .alphaBlendOp        = ToVkBlendOp(blend.alphaOp),
                .colorWriteMask      = ToVkColorMask(blend.writeMask),
            };

            blendAttachments.push_back(attachment);
        }

        VkPipelineColorBlendStateCreateInfo blend = {
            .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable   = VK_FALSE,
            .attachmentCount = static_cast<uint32_t>(blendAttachments.size()),
            .pAttachments    = blendAttachments.data(),
        };

        VkPipelineViewportStateCreateInfo viewport = {
            .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount  = 1,
        };

        const VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

        VkPipelineDynamicStateCreateInfo dynamic = {
            .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = 2,
            .pDynamicStates    = dynamicStates,
        };

        const VkStencilOpState stencil = {
            .failOp      = VK_STENCIL_OP_KEEP,
            .passOp      = VK_STENCIL_OP_KEEP,
            .depthFailOp = VK_STENCIL_OP_KEEP,
            .compareOp   = VK_COMPARE_OP_ALWAYS,
        };
        VkPipelineDepthStencilStateCreateInfo depth = {
            .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable       = desc.depthStencilState.enableDepthTest ? VK_TRUE : VK_FALSE,
            .depthWriteEnable      = desc.depthStencilState.enableDepthWrite ? VK_TRUE : VK_FALSE,
            .depthCompareOp        = ToVkCompareOp(desc.depthStencilState.depthCompareOp),
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable     = desc.depthStencilState.enableStencilTest ? VK_TRUE : VK_FALSE,
            .front                 = stencil,
            .back                  = stencil,
        };

        std::vector<VkFormat> colorFormats;
        colorFormats.reserve(desc.targetInfo.numColorTargets);

        for (uint32_t i = 0; i < desc.targetInfo.numColorTargets; ++i) {
            colorFormats.push_back(ToVkFormat(desc.targetInfo.colorTargets[i].format, mSwapchainFormat));
        }

        VkFormat depthFormat = VK_FORMAT_UNDEFINED;
        if (desc.targetInfo.hasDepthTarget) {
            depthFormat = ToVkFormat(desc.targetInfo.depthFormat, mSwapchainFormat);
        }

        VkPipelineRenderingCreateInfo rendering = {
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount    = static_cast<uint32_t>(colorFormats.size()),
            .pColorAttachmentFormats = colorFormats.data(),
            .depthAttachmentFormat   = depthFormat,
            .stencilAttachmentFormat = (depthFormat != VK_FORMAT_UNDEFINED
                                        && (depthFormat == VK_FORMAT_D24_UNORM_S8_UINT || depthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT))
                                         ? depthFormat
                                         : VK_FORMAT_UNDEFINED,
        };

        VkGraphicsPipelineCreateInfo info = {
            .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext               = &rendering,
            .stageCount          = 2,
            .pStages             = stages.data(),
            .pVertexInputState   = &vertexInput,
            .pInputAssemblyState = &assembly,
            .pViewportState      = &viewport,
            .pRasterizationState = &raster,
            .pMultisampleState   = &multisample,
            .pDepthStencilState  = &depth,
            .pColorBlendState    = &blend,
            .pDynamicState       = &dynamic,
            .layout              = state.layout,
        };

        if (!VK_CHECK_RESULT(vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &info, nullptr, &state.pipeline))) {

            vkDestroyPipelineLayout(mDevice, state.layout, nullptr);
            for (auto layout : state.setLayouts) {
                vkDestroyDescriptorSetLayout(mDevice, layout, nullptr);
            }

            return {};
        }

        const uint64_t id = NextId();
        mGraphicsPipelines.emplace(id, state);
        return GraphicsPipelineHandle{id};
    }

    void RHIDeviceVulkan::DestroyGraphicsPipeline(GraphicsPipelineHandle handle) {

        auto it = mGraphicsPipelines.find(handle.id);
        if (it == mGraphicsPipelines.end()) {
            return;
        }

        vkDeviceWaitIdle(mDevice);

        if (it->second.pipeline) {
            vkDestroyPipeline(mDevice, it->second.pipeline, nullptr);
        }

        if (it->second.layout) {
            vkDestroyPipelineLayout(mDevice, it->second.layout, nullptr);
        }

        for (VkDescriptorSetLayout layout : it->second.setLayouts) {
            if (layout) {
                vkDestroyDescriptorSetLayout(mDevice, layout, nullptr);
            }
        }

        mGraphicsPipelines.erase(it);
    }


    // Pipeline creation.


    ComputePipelineHandle RHIDeviceVulkan::CreateComputePipeline(const ComputePipelineDesc& desc) {

        VkShaderModule module  = VK_NULL_HANDLE;
        std::string entryPoint = desc.entrypoint ? desc.entrypoint : "main";

        auto shaderIt = mShaders.find(desc.computeShader.id);
        if (shaderIt != mShaders.end()) {
            if (shaderIt->second.stage != ShaderStage::Compute) {
                return {};
            }

            module = shaderIt->second.module;

            if (!desc.entrypoint) {
                entryPoint = shaderIt->second.entryPoint;
            }

        } else if (desc.spirvCode && desc.spirvSize) {
            ShaderDesc shaderDesc{};
            shaderDesc.stage      = ShaderStage::Compute;
            shaderDesc.code       = desc.spirvCode;
            shaderDesc.size       = desc.spirvSize;
            shaderDesc.entrypoint = desc.entrypoint;
            module                = CreateShaderModule(mDevice, shaderDesc.code, shaderDesc.size);
        }

        if (!module) {
            return {};
        }

        PipelineState state{};

        const uint32_t storageTextures = desc.numReadOnlyStorageTextures + desc.numReadWriteStorageTextures;

        const uint32_t storageBuffers = desc.numReadOnlyStorageBuffers + desc.numReadWriteStorageBuffers;

        state.setLayouts[2] = CreateDescriptorSetLayout(
            desc.numUniformBuffers, desc.numSamplers, storageTextures, storageBuffers, VK_SHADER_STAGE_COMPUTE_BIT);

        if (!CreatePipelineLayout(state.setLayouts, state.layout)) {
            if (shaderIt == mShaders.end()) {
                vkDestroyShaderModule(mDevice, module, nullptr);
            }

            return {};
        }

        VkPipelineShaderStageCreateInfo stage = {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = module,
            .pName  = entryPoint.c_str(),
        };

        VkComputePipelineCreateInfo info = {
            .sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage  = stage,
            .layout = state.layout,
        };

        if (!VK_CHECK_RESULT(vkCreateComputePipelines(mDevice, VK_NULL_HANDLE, 1, &info, nullptr, &state.pipeline))) {

            vkDestroyPipelineLayout(mDevice, state.layout, nullptr);
            for (auto layout : state.setLayouts) {
                if (layout) {
                    vkDestroyDescriptorSetLayout(mDevice, layout, nullptr);
                }
            }

            if (shaderIt == mShaders.end()) {
                vkDestroyShaderModule(mDevice, module, nullptr);
            }

            return {};
        }

        if (shaderIt == mShaders.end()) {
            vkDestroyShaderModule(mDevice, module, nullptr);
        }

        const uint64_t id = NextId();
        mComputePipelines.emplace(id, state);
        return ComputePipelineHandle{id};
    }

    void RHIDeviceVulkan::DestroyComputePipeline(ComputePipelineHandle handle) {

        auto it = mComputePipelines.find(handle.id);
        if (it == mComputePipelines.end()) {
            return;
        }

        vkDeviceWaitIdle(mDevice);

        if (it->second.pipeline) {
            vkDestroyPipeline(mDevice, it->second.pipeline, nullptr);
        }

        if (it->second.layout) {
            vkDestroyPipelineLayout(mDevice, it->second.layout, nullptr);
        }

        for (VkDescriptorSetLayout layout : it->second.setLayouts) {
            if (layout) {
                vkDestroyDescriptorSetLayout(mDevice, layout, nullptr);
            }
        }

        mComputePipelines.erase(it);
    }

    bool RHIDeviceVulkan::BeginImmediate(VkCommandBuffer& command) {
        VkCommandPool pool = mFrames[mCurrentFrame].commandPool;

        VkCommandBufferAllocateInfo allocation = {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = pool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        if (!VK_CHECK_RESULT(vkAllocateCommandBuffers(mDevice, &allocation, &command))) {
            return false;
        }

        VkCommandBufferBeginInfo begin = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };

        return VK_CHECK_RESULT(vkBeginCommandBuffer(command, &begin));
    }

    bool RHIDeviceVulkan::EndImmediate(VkCommandBuffer command) {
        if (!VK_CHECK_RESULT(vkEndCommandBuffer(command))) {
            return false;
        }

        VkSubmitInfo submit = {
            .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers    = &command,
        };

        if (!VK_CHECK_RESULT(vkQueueSubmit(mGraphicsQueue, 1, &submit, VK_NULL_HANDLE))) {
            return false;
        }

        if (!VK_CHECK_RESULT(vkQueueWaitIdle(mGraphicsQueue))) {
            return false;
        }

        vkFreeCommandBuffers(mDevice, mFrames[mCurrentFrame].commandPool, 1, &command);

        return true;
    }

    void RHIDeviceVulkan::TransitionImage(VkCommandBuffer command,
                                          TextureState& texture,
                                          VkImageLayout newLayout,
                                          uint32_t baseMip,
                                          uint32_t mipCount,
                                          uint32_t baseLayer,
                                          uint32_t layerCount) {

        if (texture.layout == newLayout && baseMip == 0 && mipCount == VK_REMAINING_MIP_LEVELS && baseLayer == 0
            && layerCount == VK_REMAINING_ARRAY_LAYERS) {
            return;
        }

        const VkImageMemoryBarrier barrier = {
            .sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = AccessForLayout(texture.layout),
            .dstAccessMask = AccessForLayout(newLayout),
            .oldLayout     = texture.layout,
            .newLayout     = newLayout,
            .image         = texture.image,
            .subresourceRange =
                {
                                   .aspectMask     = texture.aspect,
                                   .baseMipLevel   = baseMip,
                                   .levelCount     = mipCount,
                                   .baseArrayLayer = baseLayer,
                                   .layerCount     = layerCount,
                                   },
        };

        vkCmdPipelineBarrier(command, StageForLayout(texture.layout), StageForLayout(newLayout), 0, 0, nullptr, 0, nullptr, 1, &barrier);

        if (baseMip == 0 && mipCount == VK_REMAINING_MIP_LEVELS && baseLayer == 0 && layerCount == VK_REMAINING_ARRAY_LAYERS) {
            texture.layout = newLayout;
        }
    }

    void RHIDeviceVulkan::TransitionSwapchainImage(VkCommandBuffer command,
                                                   VkImage image,
                                                   VkImageLayout oldLayout,
                                                   VkImageLayout newLayout) {

        const VkImageMemoryBarrier barrier = MakeImageMemoryBarrier(
            image, oldLayout, newLayout, AccessForLayout(oldLayout), AccessForLayout(newLayout), VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1);

        vkCmdPipelineBarrier(command, StageForLayout(oldLayout), StageForLayout(newLayout), 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    void RHIDeviceVulkan::UploadToBuffer(BufferHandle buffer, const void* data, uint32_t size, uint32_t offset) {

        BufferState* destination = GetBuffer(buffer);
        if (!destination || !data || size == 0 || static_cast<VkDeviceSize>(offset) + size > destination->size) {
            return;
        }

        if (destination->memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
            void* mapped = nullptr;
            if (VK_CHECK_RESULT(vkMapMemory(mDevice, destination->memory, offset, size, 0, &mapped))) {
                std::memcpy(mapped, data, size);
                vkUnmapMemory(mDevice, destination->memory);
            }
            return;
        }

        BufferState staging{};
        if (!CreateStagingBuffer(size, staging)) {
            return;
        }

        void* mapped = nullptr;
        if (!VK_CHECK_RESULT(vkMapMemory(mDevice, staging.memory, 0, size, 0, &mapped))) {
            DestroyBufferState(staging);
            return;
        }

        std::memcpy(mapped, data, size);
        vkUnmapMemory(mDevice, staging.memory);

        VkCommandBuffer command = VK_NULL_HANDLE;
        if (!BeginImmediate(command)) {
            DestroyBufferState(staging);
            return;
        }

        const VkBufferCopy copy = {
            .srcOffset = 0,
            .dstOffset = offset,
            .size      = size,
        };

        vkCmdCopyBuffer(command, staging.buffer, destination->buffer, 1, &copy);
        EndImmediate(command);

        DestroyBufferState(staging);
    }

    void RHIDeviceVulkan::UploadToTexture(TextureHandle texture, const void* data, uint32_t width, uint32_t height, uint32_t mipLevel) {

        TextureState* state = GetTexture(texture);
        if (!state || !data || width == 0 || height == 0 || mipLevel >= state->mipLevels) {
            return;
        }

        if (state->sampleCount != SampleCount::Count1) {
            return;
        }

        if (texture.id == SwapchainTextureId) {
            return;
        }

        const uint32_t bpp = BytesPerPixel(state->rhiFormat);
        if (bpp == 0) {
            return;
        }

        const VkDeviceSize size = static_cast<VkDeviceSize>(width) * height * bpp;

        BufferState staging{};
        if (!CreateStagingBuffer(size, staging)) {
            return;
        }

        void* mapped = nullptr;
        if (!VK_CHECK_RESULT(vkMapMemory(mDevice, staging.memory, 0, size, 0, &mapped))) {
            DestroyBufferState(staging);
            return;
        }

        std::memcpy(mapped, data, static_cast<size_t>(size));
        vkUnmapMemory(mDevice, staging.memory);

        VkCommandBuffer command = VK_NULL_HANDLE;
        if (!BeginImmediate(command)) {
            DestroyBufferState(staging);
            return;
        }

        const VkImageLayout oldLayout = state->layout;

        const VkImageMemoryBarrier toTransfer = MakeImageMemoryBarrier(state->image,
                                                                       oldLayout,
                                                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                                       AccessForLayout(oldLayout),
                                                                       VK_ACCESS_TRANSFER_WRITE_BIT,
                                                                       state->aspect,
                                                                       mipLevel,
                                                                       1,
                                                                       0,
                                                                       state->layers);

        vkCmdPipelineBarrier(command, StageForLayout(oldLayout), VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &toTransfer);

        const VkBufferImageCopy region = {
            .imageSubresource =
                {
                                   .aspectMask     = state->aspect,
                                   .mipLevel       = mipLevel,
                                   .baseArrayLayer = 0,
                                   .layerCount     = state->layers,
                                   },
            .imageExtent = {width,                               height,                                                              1                                                },
        };

        vkCmdCopyBufferToImage(command, staging.buffer, state->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        const bool generate = state->mipLevels > 1 && mipLevel == 0;

        if (generate) {

            // Convert level 0 from transfer-dst to transfer-src before blitting.

            const VkImageMemoryBarrier baseToSrc = MakeImageMemoryBarrier(state->image,
                                                                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                                          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                                          VK_ACCESS_TRANSFER_WRITE_BIT,
                                                                          VK_ACCESS_TRANSFER_READ_BIT,
                                                                          state->aspect,
                                                                          0,
                                                                          1,
                                                                          0,
                                                                          state->layers);

            vkCmdPipelineBarrier(
                command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &baseToSrc);

            for (uint32_t level = 1; level < state->mipLevels; ++level) {
                const VkImageMemoryBarrier dst = MakeImageMemoryBarrier(state->image,
                                                                        VK_IMAGE_LAYOUT_UNDEFINED,
                                                                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                                        0,
                                                                        VK_ACCESS_TRANSFER_WRITE_BIT,
                                                                        state->aspect,
                                                                        level,
                                                                        1,
                                                                        0,
                                                                        state->layers);

                vkCmdPipelineBarrier(
                    command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &dst);

                const int32_t srcWidth  = std::max(1u, state->width >> (level - 1));
                const int32_t srcHeight = std::max(1u, state->height >> (level - 1));
                const int32_t dstWidth  = std::max(1u, state->width >> level);
                const int32_t dstHeight = std::max(1u, state->height >> level);

                const VkImageBlit blit = {
                    .srcSubresource =
                        {
                                         .aspectMask     = state->aspect,
                                         .mipLevel       = level - 1,
                                         .baseArrayLayer = 0,
                                         .layerCount     = state->layers,
                                         },
                    .srcOffsets = {{0, 0, 0}, {srcWidth, srcHeight, 1}},
                    .dstSubresource =
                        {
                                         .aspectMask     = state->aspect,
                                         .mipLevel       = level,
                                         .baseArrayLayer = 0,
                                         .layerCount     = state->layers,
                                         },
                    .dstOffsets = {{0, 0, 0}, {dstWidth, dstHeight, 1}},
                };

                vkCmdBlitImage(command,
                               state->image,
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               state->image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1,
                               &blit,
                               VK_FILTER_LINEAR);

                const VkImageMemoryBarrier src = MakeImageMemoryBarrier(state->image,
                                                                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                                        VK_ACCESS_TRANSFER_WRITE_BIT,
                                                                        VK_ACCESS_TRANSFER_READ_BIT,
                                                                        state->aspect,
                                                                        level,
                                                                        1,
                                                                        0,
                                                                        state->layers);


                // The destination becomes the source for the next mip.

                if (level + 1 < state->mipLevels) {
                    vkCmdPipelineBarrier(
                        command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &src);
                }
            }


            // Final transition for every mip, accounting for the last mip still

            // being transfer-dst while all preceding mips are transfer-src.
            for (uint32_t level = 0; level < state->mipLevels; ++level) {
                const VkImageMemoryBarrier final = MakeImageMemoryBarrier(
                    state->image,
                    level + 1 < state->mipLevels ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    HasFlag(state->usage, TextureUsage::Sampler) ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL,
                    level + 1 < state->mipLevels ? VK_ACCESS_TRANSFER_READ_BIT : VK_ACCESS_TRANSFER_WRITE_BIT,
                    VK_ACCESS_SHADER_READ_BIT,
                    state->aspect,
                    level,
                    1,
                    0,
                    state->layers);

                vkCmdPipelineBarrier(command,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                     0,
                                     0,
                                     nullptr,
                                     0,
                                     nullptr,
                                     1,
                                     &final);
            }

            state->layout =
                HasFlag(state->usage, TextureUsage::Sampler) ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;
        } else {
            const VkImageMemoryBarrier finalBarrier = MakeImageMemoryBarrier(
                state->image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                HasFlag(state->usage, TextureUsage::Sampler) ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT,
                state->aspect,
                mipLevel,
                1,
                0,
                state->layers);

            vkCmdPipelineBarrier(command,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                 0,
                                 0,
                                 nullptr,
                                 0,
                                 nullptr,
                                 1,
                                 &finalBarrier);

            state->layout = finalBarrier.newLayout;
        }

        EndImmediate(command);
        DestroyBufferState(staging);
    }

    void RHIDeviceVulkan::UploadToTextureLayer(
        TextureHandle texture, const void* data, uint32_t width, uint32_t height, uint32_t layer, uint32_t mipLevel) {

        TextureState* state = GetTexture(texture);
        if (!state || !data || layer >= state->layers || mipLevel >= state->mipLevels || width == 0 || height == 0) {
            return;
        }

        const uint32_t bpp = BytesPerPixel(state->rhiFormat);

        if (bpp == 0) {
            return;
        }

        const VkDeviceSize size = static_cast<VkDeviceSize>(width) * height * bpp;

        BufferState staging{};
        if (!CreateStagingBuffer(size, staging)) {
            return;
        }

        void* mapped = nullptr;
        if (!VK_CHECK_RESULT(vkMapMemory(mDevice, staging.memory, 0, size, 0, &mapped))) {
            DestroyBufferState(staging);
            return;
        }

        std::memcpy(mapped, data, static_cast<size_t>(size));
        vkUnmapMemory(mDevice, staging.memory);

        VkCommandBuffer command = VK_NULL_HANDLE;
        if (!BeginImmediate(command)) {
            DestroyBufferState(staging);
            return;
        }

        const VkImageMemoryBarrier toTransfer = MakeImageMemoryBarrier(state->image,
                                                                       state->layout,
                                                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                                       AccessForLayout(state->layout),
                                                                       VK_ACCESS_TRANSFER_WRITE_BIT,
                                                                       state->aspect,
                                                                       mipLevel,
                                                                       1,
                                                                       layer,
                                                                       1);

        vkCmdPipelineBarrier(
            command, StageForLayout(state->layout), VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &toTransfer);

        const VkBufferImageCopy region = {
            .imageSubresource =
                {
                                   .aspectMask     = state->aspect,
                                   .mipLevel       = mipLevel,
                                   .baseArrayLayer = layer,
                                   .layerCount     = 1,
                                   },
            .imageExtent = {width,                               height,                                                              1                                                    },
        };

        vkCmdCopyBufferToImage(command, staging.buffer, state->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        const VkImageLayout finalLayout =
            HasFlag(state->usage, TextureUsage::Sampler) ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;
        const VkImageMemoryBarrier finalBarrier = MakeImageMemoryBarrier(state->image,
                                                                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                                         finalLayout,
                                                                         VK_ACCESS_TRANSFER_WRITE_BIT,
                                                                         VK_ACCESS_SHADER_READ_BIT,
                                                                         state->aspect,
                                                                         mipLevel,
                                                                         1,
                                                                         layer,
                                                                         1);

        vkCmdPipelineBarrier(command,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &finalBarrier);

        EndImmediate(command);
        DestroyBufferState(staging);

        state->layout = finalBarrier.newLayout;
    }

    void RHIDeviceVulkan::GenerateMipmaps(TextureHandle texture) {
        TextureState* state = GetTexture(texture);
        if (!state || state->mipLevels <= 1 || state->sampleCount != SampleCount::Count1) {
            return;
        }

        VkFormatProperties properties = {
            .linearTilingFeatures = 0,
        };
        vkGetPhysicalDeviceFormatProperties(mPhysicalDevice, state->format, &properties);

        if (!(properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            return;
        }

        VkCommandBuffer command = VK_NULL_HANDLE;
        if (!BeginImmediate(command)) {
            return;
        }


        // Put the base level into transfer-src. The remaining levels become

        // transfer-dst targets as they are generated.
        const VkImageMemoryBarrier base = MakeImageMemoryBarrier(state->image,
                                                                 state->layout,
                                                                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                                 AccessForLayout(state->layout),
                                                                 VK_ACCESS_TRANSFER_READ_BIT,
                                                                 state->aspect,
                                                                 0,
                                                                 1,
                                                                 0,
                                                                 state->layers);

        vkCmdPipelineBarrier(command, StageForLayout(state->layout), VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &base);

        for (uint32_t level = 1; level < state->mipLevels; ++level) {
            const VkImageMemoryBarrier dst = MakeImageMemoryBarrier(state->image,
                                                                    VK_IMAGE_LAYOUT_UNDEFINED,
                                                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                                    0,
                                                                    VK_ACCESS_TRANSFER_WRITE_BIT,
                                                                    state->aspect,
                                                                    level,
                                                                    1,
                                                                    0,
                                                                    state->layers);

            vkCmdPipelineBarrier(
                command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &dst);

            const int32_t srcWidth  = std::max(1u, state->width >> (level - 1));
            const int32_t srcHeight = std::max(1u, state->height >> (level - 1));
            const int32_t dstWidth  = std::max(1u, state->width >> level);
            const int32_t dstHeight = std::max(1u, state->height >> level);

            const VkImageBlit blit = {
                .srcSubresource =
                    {
                                     .aspectMask     = state->aspect,
                                     .mipLevel       = level - 1,
                                     .baseArrayLayer = 0,
                                     .layerCount     = state->layers,
                                     },
                .srcOffsets = {{0, 0, 0}, {srcWidth, srcHeight, 1}},
                .dstSubresource =
                    {
                                     .aspectMask     = state->aspect,
                                     .mipLevel       = level,
                                     .baseArrayLayer = 0,
                                     .layerCount     = state->layers,
                                     },
                .dstOffsets = {{0, 0, 0}, {dstWidth, dstHeight, 1}},
            };

            vkCmdBlitImage(command,
                           state->image,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           state->image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &blit,
                           VK_FILTER_LINEAR);

            if (level + 1 < state->mipLevels) {
                const VkImageMemoryBarrier generated = MakeImageMemoryBarrier(state->image,
                                                                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                                              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                                              VK_ACCESS_TRANSFER_WRITE_BIT,
                                                                              VK_ACCESS_TRANSFER_READ_BIT,
                                                                              state->aspect,
                                                                              level,
                                                                              1,
                                                                              0,
                                                                              state->layers);

                vkCmdPipelineBarrier(
                    command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &generated);
            }
        }

        for (uint32_t level = 0; level < state->mipLevels; ++level) {
            const VkImageMemoryBarrier final = MakeImageMemoryBarrier(
                state->image,
                level + 1 < state->mipLevels ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                HasFlag(state->usage, TextureUsage::Sampler) ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL,
                level + 1 < state->mipLevels ? VK_ACCESS_TRANSFER_READ_BIT : VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT,
                state->aspect,
                level,
                1,
                0,
                state->layers);

            vkCmdPipelineBarrier(command,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                 0,
                                 0,
                                 nullptr,
                                 0,
                                 nullptr,
                                 1,
                                 &final);
        }

        EndImmediate(command);
        state->layout = HasFlag(state->usage, TextureUsage::Sampler) ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;
    }

    CommandBufferHandle RHIDeviceVulkan::BeginCommandBuffer() {
        FrameContext& frame = mFrames[mCurrentFrame];
        if (!VK_CHECK_RESULT(vkWaitForFences(mDevice, 1, &frame.fence, VK_TRUE, UINT64_MAX))) {
            return {};
        }

        for (BufferHandle buffer : frame.transientBuffers) {
            if (BufferState* state = GetBuffer(buffer)) {
                mReusableUniformBuffers[state->size].push_back(buffer);
            }
        }

        frame.transientBuffers.clear();

        for (auto commandIt = mCommands.begin(); commandIt != mCommands.end();) {
            if (commandIt->second.frameIndex == mCurrentFrame) {
                commandIt = mCommands.erase(commandIt);
            } else {
                ++commandIt;
            }
        }

        vkResetCommandPool(mDevice, frame.commandPool, 0);
        vkResetDescriptorPool(mDevice, frame.descriptorPool, 0);

        VkCommandBuffer command = frame.commandBuffer;

        VkCommandBufferBeginInfo begin = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        };

        if (!VK_CHECK_RESULT(vkBeginCommandBuffer(command, &begin))) {
            return {};
        }

        const uint64_t id = NextId();

        CommandState state{};
        state.buffer     = command;
        state.frameIndex = mCurrentFrame;

        mCommands.emplace(id, std::move(state));
        return CommandBufferHandle{id};
    }

    bool
        RHIDeviceVulkan::AcquireSwapchainTexture(CommandBufferHandle handle, TextureHandle* outTexture, uint32_t* width, uint32_t* height) {

        CommandState* command = GetCommand(handle);
        if (!command) {
            return false;
        }

        uint32_t imageIndex   = 0;
        const VkResult result = vkAcquireNextImageKHR(
            mDevice, mSwapchain, UINT64_MAX, mFrames[command->frameIndex].imageAvailable, VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            RecreateSwapchain();
            return false;
        }

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            LOG_ERROR("Failed to acquire Vulkan swapchain image (error {}).", static_cast<int>(result));
            return false;
        }

        if (mImagesInFlight[imageIndex] != VK_NULL_HANDLE && mImagesInFlight[imageIndex] != mFrames[command->frameIndex].fence) {

            vkWaitForFences(mDevice, 1, &mImagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }

        mImagesInFlight[imageIndex] = mFrames[command->frameIndex].fence;

        mCurrentImage     = imageIndex;
        command->acquired = true;

        if (outTexture) {
            *outTexture = TextureHandle{SwapchainTextureId};
        }
        if (width) {
            *width = mSwapchainExtent.width;
        }
        if (height) {
            *height = mSwapchainExtent.height;
        }

        return true;
    }

    void RHIDeviceVulkan::SubmitCommandBuffer(CommandBufferHandle handle) {
        auto it = mCommands.find(handle.id);
        if (it == mCommands.end()) {
            return;
        }

        CommandState& command = it->second;

        if (command.rendering) {
            EndRenderPass(handle);
        }

        if (!VK_CHECK_RESULT(vkEndCommandBuffer(command.buffer))) {
            mCommands.erase(it);
            return;
        }

        FrameContext& frame = mFrames[command.frameIndex];

        if (!command.acquired) {
            mCommands.erase(it);
            return;
        }

        vkResetFences(mDevice, 1, &frame.fence);

        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submit = {
            .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount   = 1,
            .pWaitSemaphores      = &frame.imageAvailable,
            .pWaitDstStageMask    = &waitStage,
            .commandBufferCount   = 1,
            .pCommandBuffers      = &command.buffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores    = &frame.renderFinished,
        };

        if (VK_CHECK_RESULT(vkQueueSubmit(mGraphicsQueue, 1, &submit, frame.fence))) {
            VkPresentInfoKHR present = {
                .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores    = &frame.renderFinished,
                .swapchainCount     = 1,
                .pSwapchains        = &mSwapchain,
                .pImageIndices      = &mCurrentImage,
            };

            const VkResult presentResult = vkQueuePresentKHR(mPresentQueue, &present);

            if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
                RecreateSwapchain();
            } else if (presentResult != VK_SUCCESS) {
                LOG_ERROR("Failed to present Vulkan swapchain image (error {}).", static_cast<int>(presentResult));
            }
        }

        mCurrentFrame = (mCurrentFrame + 1) % MaxFramesInFlight;
        mCommands.erase(it);
    }

    void RHIDeviceVulkan::UpdateBuffer(CommandBufferHandle, const void* data, uint32_t size, BufferHandle buffer, uint32_t offset) {


        // This RHI does not expose transient upload-resource lifetime. Use the

        // same synchronous staging path as UploadToBuffer() so the staging

        // allocation remains alive until the transfer has completed.

        UploadToBuffer(buffer, data, size, offset);
    }


    // Command recording and render passes.


    void RHIDeviceVulkan::BeginRenderPass(CommandBufferHandle handle, const RenderPassDesc& desc) {

        CommandState* command = GetCommand(handle);
        if (!command || command->rendering) {
            return;
        }

        command->activeColorTargets.clear();
        command->activeDepthTarget = {};

        std::vector<VkRenderingAttachmentInfo> colors;
        std::vector<TextureState*> colorTextures;
        colors.reserve(desc.numColorTargets);
        colorTextures.reserve(desc.numColorTargets);

        for (uint32_t i = 0; i < desc.numColorTargets; ++i) {
            const auto& target = desc.colorTargets[i];
            command->activeColorTargets.push_back(target.texture);

            if (target.texture.id == SwapchainTextureId || !target.texture) {
                const uint32_t index = mCurrentImage;
                if (mSwapchainLayouts[index] != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
                    TransitionSwapchainImage(
                        command->buffer, mSwapchainImages[index], mSwapchainLayouts[index], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
                    mSwapchainLayouts[index] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }

                VkRenderingAttachmentInfo attachment = {
                    .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                    .imageView   = mSwapchainImageViews[index],
                    .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .loadOp      = target.loadOp == LoadOp::Load     ? VK_ATTACHMENT_LOAD_OP_LOAD
                                 : target.loadOp == LoadOp::DontCare ? VK_ATTACHMENT_LOAD_OP_DONT_CARE
                                                                     : VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .storeOp     = target.storeOp == StoreOp::DontCare ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE,
                    .clearValue =
                        {.color = {.float32 = {target.clearColor.r, target.clearColor.g, target.clearColor.b, target.clearColor.a}}},
                };

                colors.push_back(attachment);
                colorTextures.push_back(nullptr);
            } else {
                TextureState* texture = GetTexture(target.texture);
                if (!texture) {
                    continue;
                }

                TransitionImage(command->buffer, *texture, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

                VkRenderingAttachmentInfo attachment = {
                    .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                    .imageView   = texture->view,
                    .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .loadOp      = target.loadOp == LoadOp::Load     ? VK_ATTACHMENT_LOAD_OP_LOAD
                                 : target.loadOp == LoadOp::DontCare ? VK_ATTACHMENT_LOAD_OP_DONT_CARE
                                                                     : VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .storeOp     = target.storeOp == StoreOp::DontCare ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE,
                    .clearValue =
                        {.color = {.float32 = {target.clearColor.r, target.clearColor.g, target.clearColor.b, target.clearColor.a}}},
                };

                colors.push_back(attachment);
                colorTextures.push_back(texture);
            }
        }

        VkRenderingAttachmentInfo depthAttachment = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        };

        bool hasDepth              = false;
        TextureState* depthTexture = nullptr;

        command->activeDepthTarget = desc.depthStencilTarget;

        if (desc.depthStencilTarget) {
            depthTexture = GetTexture(desc.depthStencilTarget);
            if (depthTexture) {
                hasDepth = true;
                TransitionImage(command->buffer, *depthTexture, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

                depthAttachment = {
                    .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                    .imageView   = depthTexture->view,
                    .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    .loadOp      = desc.depthLoadOp == LoadOp::Load     ? VK_ATTACHMENT_LOAD_OP_LOAD
                                 : desc.depthLoadOp == LoadOp::DontCare ? VK_ATTACHMENT_LOAD_OP_DONT_CARE
                                                                        : VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .storeOp     = desc.depthStoreOp == StoreOp::DontCare ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE,
                    .clearValue  = {.depthStencil = {.depth = desc.clearDepth, .stencil = 0}},
                };
            }
        }

        VkExtent2D renderExtent = mSwapchainExtent;
        if (!command->activeColorTargets.empty()) {
            const TextureHandle first = command->activeColorTargets.front();
            if (first && first.id != SwapchainTextureId) {
                if (const TextureState* target = GetTexture(first)) {
                    renderExtent = {target->width, target->height};
                }
            }
        } else if (depthTexture) {
            renderExtent = {depthTexture->width, depthTexture->height};
        }

        VkRenderingInfo rendering = {
            .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea           = {{0, 0}, renderExtent},
            .layerCount           = 1,
            .colorAttachmentCount = static_cast<uint32_t>(colors.size()),
            .pColorAttachments    = colors.data(),
            .pDepthAttachment     = hasDepth ? &depthAttachment : nullptr,
            .pStencilAttachment   = hasDepth && depthTexture && HasStencil(depthTexture->rhiFormat) ? &depthAttachment : nullptr,
        };

        vkCmdBeginRendering(command->buffer, &rendering);
        command->rendering = true;
    }

    void RHIDeviceVulkan::EndRenderPass(CommandBufferHandle handle) {
        CommandState* command = GetCommand(handle);
        if (!command || !command->rendering) {
            return;
        }

        vkCmdEndRendering(command->buffer);

        for (TextureHandle target : command->activeColorTargets) {
            if (target.id == SwapchainTextureId || !target) {
                continue;
            }

            TextureState* texture = GetTexture(target);
            if (!texture) {
                continue;
            }

            const VkImageLayout finalLayout =
                HasFlag(texture->usage, TextureUsage::Sampler) ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;

            TransitionImage(command->buffer, *texture, finalLayout);
        }

        if (command->activeDepthTarget) {
            if (TextureState* depth = GetTexture(command->activeDepthTarget)) {
                const VkImageLayout finalLayout = HasFlag(depth->usage, TextureUsage::Sampler)
                                                    ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                                                    : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                TransitionImage(command->buffer, *depth, finalLayout);
            }
        }


        // The swapchain image is always presented after a successful frame.

        if (mCurrentImage < mSwapchainLayouts.size()) {
            if (mSwapchainLayouts[mCurrentImage] != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
                TransitionSwapchainImage(
                    command->buffer, mSwapchainImages[mCurrentImage], mSwapchainLayouts[mCurrentImage], VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
                mSwapchainLayouts[mCurrentImage] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            }
        }

        command->activeColorTargets.clear();
        command->activeDepthTarget = {};
        command->rendering         = false;
    }

    VkDescriptorSet RHIDeviceVulkan::GetOrCreateDescriptorSet(CommandState& command, uint32_t set) {

        if (set >= 3 || !command.layout) {
            return VK_NULL_HANDLE;
        }

        PipelineState* pipeline = nullptr;

        if (!command.compute) {
            auto it = mGraphicsPipelines.find(command.boundPipelineId);
            if (it != mGraphicsPipelines.end()) {
                pipeline = &it->second;
            }
        } else {
            auto it = mComputePipelines.find(command.boundPipelineId);
            if (it != mComputePipelines.end()) {
                pipeline = &it->second;
            }
        }

        if (!pipeline || !pipeline->setLayouts[set]) {
            return VK_NULL_HANDLE;
        }

        const VkDescriptorSetLayout layout = pipeline->setLayouts[set];

        auto found = command.descriptorSets.find(layout);
        if (found != command.descriptorSets.end()) {
            return found->second;
        }

        VkDescriptorSetAllocateInfo allocation = {
            .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool     = mFrames[command.frameIndex].descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts        = &layout,
        };

        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        if (!VK_CHECK_RESULT(vkAllocateDescriptorSets(mDevice, &allocation, &descriptorSet))) {
            return VK_NULL_HANDLE;
        }

        command.descriptorSets.emplace(layout, descriptorSet);
        return descriptorSet;
    }

    void RHIDeviceVulkan::WriteBufferDescriptor(
        CommandState& command, uint32_t set, uint32_t binding, BufferHandle buffer, VkDescriptorType type) {

        BufferState* state = GetBuffer(buffer);
        if (!state) {
            return;
        }

        VkDescriptorSet descriptorSet = GetOrCreateDescriptorSet(command, set);
        if (!descriptorSet) {
            return;
        }

        const VkDescriptorBufferInfo bufferInfo = {
            .buffer = state->buffer,
            .offset = 0,
            .range  = VK_WHOLE_SIZE,
        };

        VkWriteDescriptorSet write = {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = descriptorSet,
            .dstBinding      = binding,
            .descriptorCount = 1,
            .descriptorType  = type,
            .pBufferInfo     = &bufferInfo,
        };

        vkUpdateDescriptorSets(mDevice, 1, &write, 0, nullptr);

        command.pendingSets[set] = descriptorSet;
        command.setDirty[set]    = true;
    }

    void RHIDeviceVulkan::WriteTextureDescriptor(
        CommandState& command, uint32_t set, uint32_t binding, TextureHandle texture, SamplerHandle sampler, VkDescriptorType type) {

        TextureState* textureState = GetTexture(texture);
        SamplerState* samplerState = GetSampler(sampler);

        if (!textureState || !samplerState) {
            return;
        }

        VkDescriptorSet descriptorSet = GetOrCreateDescriptorSet(command, set);
        if (!descriptorSet) {
            return;
        }

        if (textureState->layout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            if (!command.rendering) {
                TransitionImage(command.buffer, *textureState, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }
        }

        const VkDescriptorImageInfo imageInfo = {
            .sampler     = VK_NULL_HANDLE,
            .imageView   = textureState->view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        const VkDescriptorImageInfo samplerInfo = {
            .sampler = samplerState->sampler,
        };

        VkWriteDescriptorSet writes[2] = {
            {
             .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
             .dstSet          = descriptorSet,
             .dstBinding      = binding,
             .descriptorCount = 1,
             .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
             .pImageInfo      = &imageInfo,
             },
            {
             .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
             .dstSet          = descriptorSet,
             .dstBinding      = binding + 1,
             .descriptorCount = 1,
             .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER,
             .pImageInfo      = &samplerInfo,
             },
        };

        vkUpdateDescriptorSets(mDevice, 2, writes, 0, nullptr);

        command.pendingSets[set] = descriptorSet;
        command.setDirty[set]    = true;
    }

    void RHIDeviceVulkan::FlushDescriptorSets(CommandState& command) {
        for (uint32_t i = 0; i < 3; ++i) {
            if (command.setDirty[i] && command.pendingSets[i]) {
                vkCmdBindDescriptorSets(command.buffer,
                                        command.compute ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        command.layout,
                                        i,
                                        1,
                                        &command.pendingSets[i],
                                        0,
                                        nullptr);
                command.setDirty[i] = false;
            }
        }
    }

    void RHIDeviceVulkan::BindGraphicsPipeline(CommandBufferHandle handle, GraphicsPipelineHandle pipelineHandle) {

        CommandState* command   = GetCommand(handle);
        PipelineState* pipeline = GetGraphicsPipeline(pipelineHandle);

        if (!command || !pipeline) {
            return;
        }

        command->compute         = false;
        command->boundPipelineId = pipelineHandle.id;
        command->layout          = pipeline->layout;
        command->descriptorSets.clear();
        command->setDirty.fill(false);

        vkCmdBindPipeline(command->buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
    }

    void RHIDeviceVulkan::BindComputePipeline(CommandBufferHandle handle, ComputePipelineHandle pipelineHandle) {

        CommandState* command   = GetCommand(handle);
        PipelineState* pipeline = GetComputePipeline(pipelineHandle);

        if (!command || !pipeline) {
            return;
        }

        command->compute         = true;
        command->boundPipelineId = pipelineHandle.id;
        command->layout          = pipeline->layout;

        vkCmdBindPipeline(command->buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->pipeline);
    }

    void RHIDeviceVulkan::SetViewport(
        CommandBufferHandle handle, float x, float y, float width, float height, float minDepth, float maxDepth) {

        CommandState* command = GetCommand(handle);
        if (!command) {
            return;
        }

        // flip the viewport without mutating camera matrices.
        const VkViewport viewport = {
            .x        = x,
            .y        = y + height,
            .width    = width,
            .height   = -height,
            .minDepth = minDepth,
            .maxDepth = maxDepth,
        };
        vkCmdSetViewport(command->buffer, 0, 1, &viewport);
    }

    void RHIDeviceVulkan::SetScissor(CommandBufferHandle handle, int x, int y, int width, int height) {

        CommandState* command = GetCommand(handle);
        if (!command) {
            return;
        }

        const VkRect2D scissor = {
            .offset = {x, y},
            .extent = {static_cast<uint32_t>(std::max(0, width)), static_cast<uint32_t>(std::max(0, height))},
        };

        vkCmdSetScissor(command->buffer, 0, 1, &scissor);
    }

    void RHIDeviceVulkan::BindVertexBuffer(CommandBufferHandle handle, uint32_t slot, BufferHandle buffer, uint32_t offset) {

        CommandState* command = GetCommand(handle);
        BufferState* state    = GetBuffer(buffer);

        if (!command || !state) {
            return;
        }

        VkDeviceSize deviceOffset = offset;
        vkCmdBindVertexBuffers(command->buffer, slot, 1, &state->buffer, &deviceOffset);
    }

    void RHIDeviceVulkan::BindIndexBuffer(CommandBufferHandle handle, BufferHandle buffer, IndexFormat format, uint32_t offset) {

        CommandState* command = GetCommand(handle);
        BufferState* state    = GetBuffer(buffer);

        if (!command || !state) {
            return;
        }

        vkCmdBindIndexBuffer(
            command->buffer, state->buffer, offset, format == IndexFormat::UInt16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
    }

    void RHIDeviceVulkan::BindUniformBuffer(CommandBufferHandle handle, uint32_t set, uint32_t binding, BufferHandle buffer) {

        CommandState* command = GetCommand(handle);
        if (!command || set >= 3) {
            return;
        }

        WriteBufferDescriptor(*command, set, binding, buffer, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    }

    void RHIDeviceVulkan::PushVertexUniformData(CommandBufferHandle handle, uint32_t slot, const void* data, uint32_t size) {

        if (!data || size == 0) {
            return;
        }

        BufferHandle buffer = AcquireTransientUniformBuffer(size);

        if (!buffer) {
            return;
        }

        UploadToBuffer(buffer, data, size, 0);
        BindUniformBuffer(handle, 0, slot, buffer);
        if (CommandState* command = GetCommand(handle)) {
            mFrames[command->frameIndex].transientBuffers.push_back(buffer);
        }
    }

    void RHIDeviceVulkan::PushFragmentUniformData(CommandBufferHandle handle, uint32_t slot, const void* data, uint32_t size) {

        if (!data || size == 0) {
            return;
        }

        BufferHandle buffer = AcquireTransientUniformBuffer(size);

        if (!buffer) {
            return;
        }

        UploadToBuffer(buffer, data, size, 0);
        BindUniformBuffer(handle, 0, slot, buffer);
        if (CommandState* command = GetCommand(handle)) {
            mFrames[command->frameIndex].transientBuffers.push_back(buffer);
        }
    }

    void RHIDeviceVulkan::BindFragmentSampler(CommandBufferHandle handle, uint32_t binding, TextureHandle texture, SamplerHandle sampler) {

        CommandState* command = GetCommand(handle);
        if (!command) {
            return;
        }

        WriteTextureDescriptor(*command, 1, binding, texture, sampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }

    void RHIDeviceVulkan::BindVertexSampler(CommandBufferHandle handle, uint32_t binding, TextureHandle texture, SamplerHandle sampler) {

        CommandState* command = GetCommand(handle);
        if (!command) {
            return;
        }

        WriteTextureDescriptor(*command, 0, binding, texture, sampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }

    void RHIDeviceVulkan::DrawPrimitives(
        CommandBufferHandle handle, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {

        CommandState* command = GetCommand(handle);
        if (!command) {
            return;
        }

        FlushDescriptorSets(*command);
        vkCmdDraw(command->buffer, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void RHIDeviceVulkan::DrawIndexed(CommandBufferHandle handle,
                                      uint32_t indexCount,
                                      uint32_t instanceCount,
                                      uint32_t firstIndex,
                                      int32_t vertexOffset,
                                      uint32_t firstInstance) {

        CommandState* command = GetCommand(handle);
        if (!command) {
            return;
        }

        FlushDescriptorSets(*command);
        vkCmdDrawIndexed(command->buffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void RHIDeviceVulkan::BeginComputePass(CommandBufferHandle handle, const ComputePassDesc& desc) {

        CommandState* command = GetCommand(handle);
        if (!command) {
            return;
        }

        command->compute = true;


        // Compute storage resources are transitioned to GENERAL here.

        for (uint32_t i = 0; i < desc.numReadWriteTextures; ++i) {
            TextureState* texture = GetTexture(desc.readWriteTextures[i]);
            if (!texture) {
                continue;
            }

            TransitionImage(command->buffer, *texture, VK_IMAGE_LAYOUT_GENERAL);
        }
    }

    void RHIDeviceVulkan::EndComputePass(CommandBufferHandle handle) {
        CommandState* command = GetCommand(handle);
        if (!command) {
            return;
        }


        // Resource state remains GENERAL for storage images. A subsequent

        // graphics/texture use performs the required transition.
    }

    void RHIDeviceVulkan::BindComputeStorageBuffer(CommandBufferHandle handle, uint32_t binding, BufferHandle buffer) {

        CommandState* command = GetCommand(handle);
        if (!command) {
            return;
        }

        WriteBufferDescriptor(*command, 2, binding, buffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    }

    void RHIDeviceVulkan::BindComputeStorageTexture(CommandBufferHandle handle, uint32_t binding, TextureHandle texture) {

        CommandState* command = GetCommand(handle);
        TextureState* state   = GetTexture(texture);

        if (!command || !state) {
            return;
        }

        TransitionImage(command->buffer, *state, VK_IMAGE_LAYOUT_GENERAL);

        VkDescriptorSet descriptorSet = GetOrCreateDescriptorSet(*command, 2);
        if (!descriptorSet) {
            return;
        }

        const VkDescriptorImageInfo imageInfo = {
            .sampler     = VK_NULL_HANDLE,
            .imageView   = state->view,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        };

        VkWriteDescriptorSet write = {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = descriptorSet,
            .dstBinding      = binding,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo      = &imageInfo,
        };

        vkUpdateDescriptorSets(mDevice, 1, &write, 0, nullptr);

        vkCmdBindDescriptorSets(command->buffer, VK_PIPELINE_BIND_POINT_COMPUTE, command->layout, 2, 1, &descriptorSet, 0, nullptr);
    }

    void RHIDeviceVulkan::PushComputeUniformData(CommandBufferHandle handle, uint32_t slot, const void* data, uint32_t size) {

        if (!data || size == 0) {
            return;
        }

        BufferHandle buffer = CreateBuffer({BufferUsage::Uniform, size});

        if (!buffer) {
            return;
        }

        UploadToBuffer(buffer, data, size, 0);
        BindUniformBuffer(handle, 2, slot, buffer);
        if (CommandState* command = GetCommand(handle)) {
            mFrames[command->frameIndex].transientBuffers.push_back(buffer);
        }
    }

    void RHIDeviceVulkan::DispatchCompute(CommandBufferHandle handle, uint32_t x, uint32_t y, uint32_t z) {

        CommandState* command = GetCommand(handle);
        if (!command) {
            return;
        }

        FlushDescriptorSets(*command);
        vkCmdDispatch(command->buffer, x, y, z);
    }

    void RHIDeviceVulkan::WaitForIdle() {
        if (mDevice) {
            vkDeviceWaitIdle(mDevice);
        }
    }

    void RHIDeviceVulkan::PushDebugGroup(CommandBufferHandle handle, const char* name) {

        CommandState* command = GetCommand(handle);
        if (!command || !mCmdBeginDebugUtilsLabel) {
            return;
        }

        VkDebugUtilsLabelEXT label = {
            .sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
            .pLabelName = name ? name : "Golias",
        };
        mCmdBeginDebugUtilsLabel(command->buffer, &label);
    }

    void RHIDeviceVulkan::PopDebugGroup(CommandBufferHandle handle) {
        CommandState* command = GetCommand(handle);
        if (!command || !mCmdEndDebugUtilsLabel) {
            return;
        }

        mCmdEndDebugUtilsLabel(command->buffer);
    }

    void RHIDeviceVulkan::InsertDebugLabel(CommandBufferHandle handle, const char* name) {

        CommandState* command = GetCommand(handle);
        if (!command || !mCmdInsertDebugUtilsLabel) {
            return;
        }

        VkDebugUtilsLabelEXT label = {
            .sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
            .pLabelName = name ? name : "Golias",
        };
        mCmdInsertDebugUtilsLabel(command->buffer, &label);
    }

    void RHIDeviceVulkan::SetObjectName(VkObjectType type, uint64_t object, const char* name) {

        if (!mSetDebugUtilsObjectName || !object || !name) {
            return;
        }

        VkDebugUtilsObjectNameInfoEXT info = {
            .sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .objectType   = type,
            .objectHandle = object,
            .pObjectName  = name,
        };

        mSetDebugUtilsObjectName(mDevice, &info);
    }

    RHIDeviceVulkan::TextureState* RHIDeviceVulkan::GetTexture(TextureHandle handle) {
        if (!handle || handle.id == SwapchainTextureId) {
            return nullptr;
        }

        auto it = mTextures.find(handle.id);
        return it == mTextures.end() ? nullptr : &it->second;
    }

    const RHIDeviceVulkan::TextureState* RHIDeviceVulkan::GetTexture(TextureHandle handle) const {
        if (!handle || handle.id == SwapchainTextureId) {
            return nullptr;
        }

        auto it = mTextures.find(handle.id);
        return it == mTextures.end() ? nullptr : &it->second;
    }

    RHIDeviceVulkan::BufferState* RHIDeviceVulkan::GetBuffer(BufferHandle handle) {
        auto it = mBuffers.find(handle.id);
        return it == mBuffers.end() ? nullptr : &it->second;
    }

    const RHIDeviceVulkan::BufferState* RHIDeviceVulkan::GetBuffer(BufferHandle handle) const {
        auto it = mBuffers.find(handle.id);
        return it == mBuffers.end() ? nullptr : &it->second;
    }

    RHIDeviceVulkan::SamplerState* RHIDeviceVulkan::GetSampler(SamplerHandle handle) {
        auto it = mSamplers.find(handle.id);
        return it == mSamplers.end() ? nullptr : &it->second;
    }

    RHIDeviceVulkan::PipelineState* RHIDeviceVulkan::GetGraphicsPipeline(GraphicsPipelineHandle handle) {
        auto it = mGraphicsPipelines.find(handle.id);
        return it == mGraphicsPipelines.end() ? nullptr : &it->second;
    }

    RHIDeviceVulkan::PipelineState* RHIDeviceVulkan::GetComputePipeline(ComputePipelineHandle handle) {
        auto it = mComputePipelines.find(handle.id);
        return it == mComputePipelines.end() ? nullptr : &it->second;
    }

    RHIDeviceVulkan::CommandState* RHIDeviceVulkan::GetCommand(CommandBufferHandle handle) {
        auto it = mCommands.find(handle.id);
        return it == mCommands.end() ? nullptr : &it->second;
    }

    RHIDeviceVulkan::~RHIDeviceVulkan() {
        if (mDevice) {
            vkDeviceWaitIdle(mDevice);
        }

        mCommands.clear();

        for (auto& [id, pipeline] : mGraphicsPipelines) {
            if (pipeline.pipeline) {
                vkDestroyPipeline(mDevice, pipeline.pipeline, nullptr);
            }

            if (pipeline.layout) {
                vkDestroyPipelineLayout(mDevice, pipeline.layout, nullptr);
            }

            for (VkDescriptorSetLayout layout : pipeline.setLayouts) {
                if (layout) {
                    vkDestroyDescriptorSetLayout(mDevice, layout, nullptr);
                }
            }
        }
        mGraphicsPipelines.clear();

        for (auto& [id, pipeline] : mComputePipelines) {
            if (pipeline.pipeline) {
                vkDestroyPipeline(mDevice, pipeline.pipeline, nullptr);
            }

            if (pipeline.layout) {
                vkDestroyPipelineLayout(mDevice, pipeline.layout, nullptr);
            }

            for (VkDescriptorSetLayout layout : pipeline.setLayouts) {
                if (layout) {
                    vkDestroyDescriptorSetLayout(mDevice, layout, nullptr);
                }
            }
        }

        mComputePipelines.clear();

        for (auto& [id, shader] : mShaders) {
            if (shader.module) {
                vkDestroyShaderModule(mDevice, shader.module, nullptr);
            }
        }

        mShaders.clear();

        for (auto& [id, sampler] : mSamplers) {
            if (sampler.sampler) {
                vkDestroySampler(mDevice, sampler.sampler, nullptr);
            }
        }

        mSamplers.clear();

        for (auto& [id, texture] : mTextures) {
            DestroyTextureState(texture);
        }

        mTextures.clear();

        for (auto& [id, buffer] : mBuffers) {
            DestroyBufferState(buffer);
        }

        mBuffers.clear();

        if (mDevice) {
            DestroyFrameContexts();
        }

        DestroySwapchain();

        if (mSurface) {
            vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
        }

        if (mDevice) {
            vkDestroyDevice(mDevice, nullptr);
        }

        if (mDebugMessenger) {
            DestroyDebugMessenger(mInstance, mDebugMessenger);
        }

        if (mInstance) {
            vkDestroyInstance(mInstance, nullptr);
        }
    }

} // namespace golias
