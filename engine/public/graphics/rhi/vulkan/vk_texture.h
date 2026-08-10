#pragma once
#include "graphics/rhi/rhi_texture.h"
#include "vk_common.h"

namespace golias {

    class VulkanDevice;

    class VulkanTexture : public Texture {
    public:
        VulkanTexture(Ref<VulkanDevice> device, const TextureDesc& desc);
        ~VulkanTexture() override;

        VulkanTexture(const VulkanTexture&)            = delete;
        VulkanTexture& operator=(const VulkanTexture&) = delete;

        static Ref<VulkanTexture> CreateFromFile(Ref<VulkanDevice> device, const String& path, const TextureDesc& desc);

        // Convenience overload: RGBA8 shader-resource texture, no mips.
        static Ref<VulkanTexture> CreateFromFile(Ref<VulkanDevice> device, const String& path);

        uint32_t GetWidth() const override;

        uint32_t GetHeight() const override;

        TextureFormat GetFormat() const override;

        uint32_t GetArraySize() const override;

        uint32_t GetSampleCount() const override;

        uint32_t GetMipLevels() const override;

        TextureUsage GetUsage() const override;

        VkFormat GetVulkanFormat() const;

        VkImage GetImage() const;

        VkImageView GetImageView() const;

    private:
        static VkFormat ToVulkanFormat(TextureFormat format);

        static VkImageUsageFlags ToVulkanUsage(TextureUsage usage);

        void CreateImage(const TextureDesc& desc);

        void CreateImageView();

        void UploadData(const void* data, size_t dataSize);

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
        uint32_t mMipLevels          = 1;
    };

} // namespace golias
