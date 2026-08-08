#pragma once
#include "vk_common.h"
#include "graphics/rhi/rhi_shader.h"

namespace golias {

    class VulkanDevice;

    VkShaderStageFlagBits ConvertShaderStage(ShaderStage stage);

    class VulkanShader : public Shader {
    public:
        VulkanShader() = default;
        ~VulkanShader() override;

        static Ref<VulkanShader> CreateFromFile(Ref<VulkanDevice> device, const ShaderDesc& desc);

        ShaderStage GetStage() const override;

        VkShaderModule GetHandle() const;

        VkShaderStageFlagBits GetStageFlagBits() const;

        const char* GetEntryPoint() const;

    private:
        std::vector<char> ReadFile(const std::string& path);

    private:
        Ref<VulkanDevice> mDevice;
        VkShaderModule mShaderModule = VK_NULL_HANDLE;

        ShaderStage mStage = ShaderStage::Vertex;
        std::string mEntryPoint;
    };
} // namespace golias
