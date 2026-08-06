#pragma once

#include "vk_common.h"

namespace golias {

    class VulkanInstance {
    public:
        explicit VulkanInstance(bool enableValidationLayers = true);
        ~VulkanInstance();

        VkInstance GetHandle() const {
            return mInstance;
        }

    private:
        std::vector<const char*> GetRequiredExtensions() const;
        bool CheckValidationLayerSupport() const;
        void SetupDebugMessenger();

    private:
        VkInstance mInstance = VK_NULL_HANDLE;

        VkDebugUtilsMessengerEXT mDebugMessenger = VK_NULL_HANDLE;
        bool mEnableValidationLayers             = true;

        const std::vector<const char*> mValidationLayers = {"VK_LAYER_KHRONOS_validation"};
    };
} // namespace golias
