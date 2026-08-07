#include "graphics/rhi/vulkan/vk_shader.h"

#include "graphics/rhi/vulkan/vk_device.h"

namespace golias {

    VkShaderStageFlagBits VulkanShader::ConvertShaderStage(ShaderStage stage) const {
        switch (stage) {
        case ShaderStage::Vertex:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case ShaderStage::Fragment:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case ShaderStage::Geometry:
            return VK_SHADER_STAGE_GEOMETRY_BIT;
        case ShaderStage::Compute:
            return VK_SHADER_STAGE_COMPUTE_BIT;
        default:
            LOG_FATAL("Unknown shader stage");
            return VK_SHADER_STAGE_VERTEX_BIT;
        }
    }

    VkShaderModule VulkanShader::GetHandle() const {
        return mShaderModule;
    }

    VkShaderStageFlagBits VulkanShader::GetStage() const {
        return mStage;
    }

    const char* VulkanShader::GetEntryPoint() const {
        return mEntryPoint.c_str();
    }


    std::vector<char> VulkanShader::ReadFile(const std::string& path) {
        std::ifstream file(path, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            LOG_FATAL("Failed to open shader file: {}", path);
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

    VulkanShader::~VulkanShader() {
        if (mShaderModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(mDevice->GetHandle(), mShaderModule, nullptr);
            mShaderModule = VK_NULL_HANDLE;
        }
    }

    Ref<VulkanShader> VulkanShader::CreateFromFile(Ref<VulkanDevice> device, ShaderDesc desc) {
        Ref<VulkanShader> shader = std::make_shared<VulkanShader>();
        shader->mDevice = device;
        shader->mEntryPoint = desc.entryPoint;
        shader->mStage = shader->ConvertShaderStage(desc.stage);
        std::vector<char> shaderCode = shader->ReadFile(desc.path);

        VkShaderModuleCreateInfo createInfo = {
            .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext    = nullptr,
            .flags    = 0,
            .codeSize = shaderCode.size(),
            .pCode    = reinterpret_cast<const uint32_t*>(shaderCode.data()),
        };

        VkResult result = vkCreateShaderModule(shader->mDevice->GetHandle(), &createInfo, nullptr, &shader->mShaderModule);
        VK_CHECK_RESULT(result);

        LOG_INFO("Created shader module for {} | entry point {} | stage {}", desc.path, desc.entryPoint, string_VkShaderStageFlagBits(shader->mStage));

        return shader;
    }

} // namespace golias
