#pragma once
#include "vk_common.h"

namespace golias {

    class VulkanDevice;

    class VulkanShader {
    public:
        VulkanShader(Ref<VulkanDevice> device, const std::string& fileName, const std::string& entryPoint, VkShaderStageFlagBits stage);
        ~VulkanShader();

        VkShaderModule GetHandle() const {
            return mShaderModule;
        }

        VkShaderStageFlagBits GetStage() const {
            return mStage;
        }

        const char* GetEntryPoint() const {
            return mEntryPoint.c_str();
        }

    private:
        std::vector<char> ReadFile(const std::string& fileName) {
            std::ifstream file(fileName, std::ios::ate | std::ios::binary);

            if (!file.is_open()) {
                LOG_FATAL("Failed to open shader file: {}", fileName);
                file.close();
                return std::vector<char>();
            }

            size_t fileSize = static_cast<size_t>(file.tellg());
            std::vector<char> buffer(fileSize);
            file.seekg(0);
            file.read(buffer.data(), fileSize);
            file.close();
            return buffer;
        }

    private:
        Ref<VulkanDevice> mDevice;
        VkShaderModule mShaderModule = VK_NULL_HANDLE;

        VkShaderStageFlagBits mStage = VK_SHADER_STAGE_VERTEX_BIT;
        std::string mEntryPoint;
    };
} // namespace golias
