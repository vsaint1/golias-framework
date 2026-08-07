#pragma once
#include "vk_common.h"

namespace golias {

    class VulkanDevice;

    enum class ShaderStage { Vertex, Fragment, Geometry, Compute };

    enum class ShaderSourceType { GLSL, HLSL, SPIRV };

    struct ShaderDesc {
        std::string path;

        ShaderStage stage = ShaderStage::Vertex;

        std::string entryPoint = "main";

        ShaderSourceType source = ShaderSourceType::SPIRV;
    };



    class VulkanShader  {
    public:
        VulkanShader()= default;
        ~VulkanShader();

        static Ref<VulkanShader> CreateFromFile(Ref<VulkanDevice> device, ShaderDesc desc);

        VkShaderModule GetHandle() const;

        VkShaderStageFlagBits GetStage() const;

        const char* GetEntryPoint() const;

    private:
        std::vector<char> ReadFile(const std::string& path);

        VkShaderStageFlagBits ConvertShaderStage(ShaderStage stage) const;

    private:
        Ref<VulkanDevice> mDevice;
        VkShaderModule mShaderModule = VK_NULL_HANDLE;

        VkShaderStageFlagBits mStage = VK_SHADER_STAGE_VERTEX_BIT;
        std::string mEntryPoint;
    };
} // namespace golias
