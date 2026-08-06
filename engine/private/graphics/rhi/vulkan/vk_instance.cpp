#include "graphics/rhi/vulkan/vk_instance.h"

namespace golias {

    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                         VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                         const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                         void* pUserData) {

        LOG_WARN("Validation layer: {}", pCallbackData->pMessage);

        return VK_FALSE;
    }

    static VkResult create_debug_utils_messenger_ext(VkInstance instance,
                                                     const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                                     const VkAllocationCallbacks* pAllocator,
                                                     VkDebugUtilsMessengerEXT* pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    static void destroy_debug_utils_messenger_ext(VkInstance instance,
                                                  VkDebugUtilsMessengerEXT debugMessenger,
                                                  const VkAllocationCallbacks* pAllocator) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }

    VulkanInstance::VulkanInstance(bool enableValidationLayers) : mEnableValidationLayers(enableValidationLayers) {
        uint32_t supportedVersion = VK_API_VERSION_1_0;
        VkResult result           = vkEnumerateInstanceVersion(&supportedVersion);
        VK_CHECK_RESULT(result);

        if (supportedVersion < VK_API_VERSION_1_3) {
            LOG_FATAL("This Vulkan version is not supported. Minimum required version is 1.3");
        }

        VkApplicationInfo appInfo = {
            .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext              = nullptr,
            .pApplicationName   = "GOLIAS_ENGINE_APP",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName        = "GOLIAS_ENGINE",
            .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion         = VK_API_VERSION_1_3,
        };


        auto extensions = GetRequiredExtensions();

        VkInstanceCreateInfo instanceCreateInfo = {
            .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext                   = nullptr,
            .pApplicationInfo        = &appInfo,
            .enabledExtensionCount   = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data(),
        };

        if (mEnableValidationLayers && CheckValidationLayerSupport()) {
            instanceCreateInfo.enabledLayerCount   = static_cast<uint32_t>(mValidationLayers.size());
            instanceCreateInfo.ppEnabledLayerNames = mValidationLayers.data();
        } else {
            instanceCreateInfo.enabledLayerCount   = 0;
            instanceCreateInfo.ppEnabledLayerNames = nullptr;
        }

        result = vkCreateInstance(&instanceCreateInfo, nullptr, &mInstance);
        VK_CHECK_RESULT(result);

        SetupDebugMessenger();

        LOG_INFO("Created successfully.");
    }

    std::vector<const char*> VulkanInstance::GetRequiredExtensions() const {
        uint32_t extensionCount     = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + extensionCount);

        // extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

        if (mEnableValidationLayers && CheckValidationLayerSupport()) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    void VulkanInstance::SetupDebugMessenger() {

        if (!mEnableValidationLayers) {
            return;
        }
        
        VkDebugUtilsMessengerCreateInfoEXT createInfo = {
            .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .pNext           = nullptr,
            .flags           = 0,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = debug_callback,
            .pUserData       = nullptr,
        };

        VkResult result = create_debug_utils_messenger_ext(mInstance, &createInfo, nullptr, &mDebugMessenger);
        VK_CHECK_RESULT(result);

        LOG_WARN("Validation layers enabled.");
    }


    bool VulkanInstance::CheckValidationLayerSupport() const {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : mValidationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }


    VulkanInstance::~VulkanInstance() {
        if (mEnableValidationLayers) {
            destroy_debug_utils_messenger_ext(mInstance, mDebugMessenger, nullptr);
        }

        vkDestroyInstance(mInstance, nullptr);
    }
} // namespace golias
