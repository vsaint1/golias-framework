#include "graphics/rhi/vulkan/vk_texture.h"

#include "graphics/rhi/vulkan/vk_device.h"

namespace golias {

    namespace {

        bool IsDepthFormat(TextureFormat format) {
            switch (format) {
            case TextureFormat::Depth16:
            case TextureFormat::Depth24Stencil8:
            case TextureFormat::Depth32Float:
                return true;
            default:
                return false;
            }
        }

    } // namespace

    VulkanTexture::VulkanTexture(Ref<VulkanDevice> device,
                                 uint32_t width,
                                 uint32_t height,
                                 TextureFormat format,
                                 TextureUsage usage,
                                 uint32_t arraySize,
                                 uint32_t sampleCount)
        : mDevice(device), mWidth(width), mHeight(height), mTextureFormat(format), mUsage(usage), mArraySize(arraySize),
          mSampleCount(sampleCount) {

        mFormat = ToVulkanFormat(format);

        VkImageCreateInfo imageInfo = {
            .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType     = VK_IMAGE_TYPE_2D,
            .format        = mFormat,
            .extent        = {mWidth, mHeight, 1},
            .mipLevels     = 1,
            .arrayLayers   = mArraySize,
            .samples       = static_cast<VkSampleCountFlagBits>(mSampleCount),
            .tiling        = VK_IMAGE_TILING_OPTIMAL,
            .usage         = ToVulkanUsage(usage),
            .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        VK_CHECK_RESULT(vkCreateImage(mDevice->GetHandle(), &imageInfo, nullptr, &mImage));

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(mDevice->GetHandle(), mImage, &memRequirements);

        uint32_t memoryTypeIndex = mDevice->FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkMemoryAllocateInfo allocInfo = {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize  = memRequirements.size,
            .memoryTypeIndex = memoryTypeIndex,
        };

        VK_CHECK_RESULT(vkAllocateMemory(mDevice->GetHandle(), &allocInfo, nullptr, &mMemory));
        VK_CHECK_RESULT(vkBindImageMemory(mDevice->GetHandle(), mImage, mMemory, 0));

        VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        if (IsDepthFormat(format)) {
            aspectMask =
                mDevice->HasStencilComponent(mFormat) ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;
        }

        VkImageViewCreateInfo viewInfo = {
            .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image    = mImage,
            .viewType = (mArraySize > 1) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D,
            .format   = mFormat,
            .subresourceRange =
                {
                                   .aspectMask     = aspectMask,
                                   .baseMipLevel   = 0,
                                   .levelCount     = 1,
                                   .baseArrayLayer = 0,
                                   .layerCount     = mArraySize,
                                   },
        };

        VK_CHECK_RESULT(vkCreateImageView(mDevice->GetHandle(), &viewInfo, nullptr, &mImageView));

        LOG_INFO(
            "Created texture: {}x{} array={} samples={} format={}", mWidth, mHeight, mArraySize, mSampleCount, string_VkFormat(mFormat));
    }

    VulkanTexture::~VulkanTexture() {
        if (mImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(mDevice->GetHandle(), mImageView, nullptr);
            mImageView = VK_NULL_HANDLE;
        }

        if (mImage != VK_NULL_HANDLE) {
            vkDestroyImage(mDevice->GetHandle(), mImage, nullptr);
            mImage = VK_NULL_HANDLE;
        }

        if (mMemory != VK_NULL_HANDLE) {
            vkFreeMemory(mDevice->GetHandle(), mMemory, nullptr);
            mMemory = VK_NULL_HANDLE;
        }
    }

    uint32_t VulkanTexture::GetWidth() const {
        return mWidth;
    }

    uint32_t VulkanTexture::GetHeight() const {
        return mHeight;
    }

    TextureFormat VulkanTexture::GetFormat() const {
        return mTextureFormat;
    }

    uint32_t VulkanTexture::GetArraySize() const {
        return mArraySize;
    }

    uint32_t VulkanTexture::GetSampleCount() const {
        return mSampleCount;
    }

    TextureUsage VulkanTexture::GetUsage() const {
        return mUsage;
    }

    VkFormat VulkanTexture::GetVulkanFormat() const {
        return mFormat;
    }

    VkImage VulkanTexture::GetImage() const {
        return mImage;
    }

    VkImageView VulkanTexture::GetImageView() const {
        return mImageView;
    }

    VkFormat VulkanTexture::ToVulkanFormat(TextureFormat format) {
        switch (format) {
        case TextureFormat::R8:
            return VK_FORMAT_R8_UNORM;
        case TextureFormat::RG8:
            return VK_FORMAT_R8G8_UNORM;
        case TextureFormat::RGBA8:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case TextureFormat::RGBA8SRGB:
            return VK_FORMAT_R8G8B8A8_SRGB;
        case TextureFormat::R16F:
            return VK_FORMAT_R16_SFLOAT;
        case TextureFormat::RG16F:
            return VK_FORMAT_R16G16_SFLOAT;
        case TextureFormat::RGBA16F:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        case TextureFormat::RGBA32F:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        case TextureFormat::Depth16:
            return VK_FORMAT_D16_UNORM;
        case TextureFormat::Depth24Stencil8:
            return VK_FORMAT_D24_UNORM_S8_UINT;
        case TextureFormat::Depth32Float:
            return VK_FORMAT_D32_SFLOAT;
        }

        LOG_FATAL("Unhandled texture format.");
        return VK_FORMAT_UNDEFINED;
    }

    VkImageUsageFlags VulkanTexture::ToVulkanUsage(TextureUsage usage) {
        VkImageUsageFlags flags = 0;

        if (HasFlag(usage, TextureUsage::RenderTarget)) {
            flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }

        if (HasFlag(usage, TextureUsage::DepthStencil)) {
            flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }

        if (HasFlag(usage, TextureUsage::ShaderResource)) {
            flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }

        if (HasFlag(usage, TextureUsage::UnorderedAccess)) {
            flags |= VK_IMAGE_USAGE_STORAGE_BIT;
        }

        if (flags == 0) {
            LOG_FATAL("Texture must be created with at least one usage flag.");
        }

        return flags;
    }

} // namespace golias
