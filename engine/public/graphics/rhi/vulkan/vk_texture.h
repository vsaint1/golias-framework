#pragma once
#include "graphics/rhi/rhi_texture.h"
#include "vk_common.h"

namespace golias {

    class VulkanDevice;


    class VulkanTexture : public Texture {
    public:
        VulkanTexture(Ref<VulkanDevice> device,
                      uint32_t width,
                      uint32_t height,
                      TextureFormat format,
                      TextureUsage usage   = TextureUsage::RenderTarget,
                      uint32_t arraySize   = 1,
                      uint32_t sampleCount = 1);
        ~VulkanTexture() override;

        VulkanTexture(const VulkanTexture&)            = delete;
        VulkanTexture& operator=(const VulkanTexture&) = delete;

        uint32_t GetWidth() const override;

        uint32_t GetHeight() const override;

        TextureFormat GetFormat() const override;

        uint32_t GetArraySize() const override;

        uint32_t GetSampleCount() const override;

        TextureUsage GetUsage() const override;

        VkFormat GetVulkanFormat() const;

        VkImage GetImage() const;

        VkImageView GetImageView() const;

    private:
        static VkFormat ToVulkanFormat(TextureFormat format);

        static VkImageUsageFlags ToVulkanUsage(TextureUsage usage);

    private:
        Ref<VulkanDevice> mDevice = nullptr;

        VkImage mImage         = VK_NULL_HANDLE;
        VkDeviceMemory mMemory = VK_NULL_HANDLE;
        VkImageView mImageView = VK_NULL_HANDLE;

        VkFormat mFormat             = VK_FORMAT_UNDEFINED;
        TextureFormat mTextureFormat = TextureFormat::RGBA8;
        TextureUsage mUsage          = TextureUsage::RenderTarget;
        uint32_t mWidth              = 0;
        uint32_t mHeight             = 0;
        uint32_t mArraySize          = 1;
        uint32_t mSampleCount        = 1;
    };

} // namespace golias
