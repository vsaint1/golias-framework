#include "graphics/rhi/vulkan/vk_shader.h"
#include "graphics/rhi/vulkan/vk_device.h"

namespace golias {

    VulkanShader::VulkanShader(Ref<VulkanDevice> device,
                               const std::string& fileName,
                               const std::string& entryPoint,
                               VkShaderStageFlagBits stage)
        : mDevice(device), mStage(stage), mEntryPoint(entryPoint) {

        std::vector<char> shaderCode = ReadFile(fileName);

        VkShaderModuleCreateInfo createInfo = {
            .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext    = nullptr,
            .flags    = 0,
            .codeSize = shaderCode.size(),
            .pCode    = reinterpret_cast<const uint32_t*>(shaderCode.data()),
        };

        VkResult result = vkCreateShaderModule(mDevice->GetHandle(), &createInfo, nullptr, &mShaderModule);
        VK_CHECK_RESULT(result);

        LOG_INFO("Created shader module for {} | entry point {} | stage {}", fileName, entryPoint, string_VkShaderStageFlagBits(stage));
    }

    VulkanShader::~VulkanShader() {
        if (mShaderModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(mDevice->GetHandle(), mShaderModule, nullptr);
            mShaderModule = VK_NULL_HANDLE;
        }
    }

} // namespace golias
