#pragma once

#include "graphics/rhi/rhi_device.h"
#include <vulkan/vulkan.h>


namespace golias {

    class Window;

    class RHIDeviceVulkan final : public RHIDevice {
    public:
        RHIDeviceVulkan(Window* window, bool debug = true);
        ~RHIDeviceVulkan() override;


        RHIBackend GetBackend() const override {
            return RHIBackend::ForwardPlus;
        }

        RHIDeviceInfo GetDeviceInfo() const override {
            return mInfo;
        }

        RHICapabilities GetCapabilities() const override {
            return mCapabilities;
        }

        const char* GetDriverName() const override {
            return "Vulkan";
        }

        void SetVsyncEnabled(bool enabled) override;

        bool IsVsyncEnabled() const override {
            return mVsyncEnabled;
        }

        TextureHandle CreateTexture(const TextureDesc&) override;
        void DestroyTexture(TextureHandle) override;

        BufferHandle CreateBuffer(const BufferDesc&) override;
        void DestroyBuffer(BufferHandle) override;

        SamplerHandle CreateSampler(const SamplerDesc&) override;
        void DestroySampler(SamplerHandle) override;

        ShaderHandle CreateShader(const ShaderDesc&) override;
        void DestroyShader(ShaderHandle) override;

        GraphicsPipelineHandle CreateGraphicsPipeline(const GraphicsPipelineDesc&) override;
        void DestroyGraphicsPipeline(GraphicsPipelineHandle) override;

        ComputePipelineHandle CreateComputePipeline(const ComputePipelineDesc&) override;
        void DestroyComputePipeline(ComputePipelineHandle) override;

        void UploadToBuffer(BufferHandle, const void*, uint32_t, uint32_t) override;
        void UploadToTexture(TextureHandle, const void*, uint32_t, uint32_t, uint32_t) override;
        void UploadToTextureLayer(TextureHandle, const void*, uint32_t, uint32_t, uint32_t, uint32_t) override;
        void GenerateMipmaps(TextureHandle) override;

        CommandBufferHandle BeginCommandBuffer() override;
        void SubmitCommandBuffer(CommandBufferHandle) override;

        TextureFormat GetSwapchainFormat() const override {
            return TextureFormat::Swapchain;
        }
        
        bool AcquireSwapchainTexture(CommandBufferHandle, TextureHandle* outTexture, uint32_t* width, uint32_t* height) override;

        void UpdateBuffer(CommandBufferHandle, const void*, uint32_t, BufferHandle, uint32_t) override;

        void BeginRenderPass(CommandBufferHandle, const RenderPassDesc&) override;
        void EndRenderPass(CommandBufferHandle) override;

        void BindGraphicsPipeline(CommandBufferHandle, GraphicsPipelineHandle) override;
        void SetViewport(CommandBufferHandle, float, float, float, float, float, float) override;
        void SetScissor(CommandBufferHandle, int, int, int, int) override;

        void BindVertexBuffer(CommandBufferHandle, uint32_t, BufferHandle, uint32_t) override;
        void BindIndexBuffer(CommandBufferHandle, BufferHandle, IndexFormat, uint32_t) override;
        void BindUniformBuffer(CommandBufferHandle, uint32_t, uint32_t, BufferHandle) override;

        void PushVertexUniformData(CommandBufferHandle, uint32_t, const void*, uint32_t) override;
        void PushFragmentUniformData(CommandBufferHandle, uint32_t, const void*, uint32_t) override;

        void BindFragmentSampler(CommandBufferHandle, uint32_t, TextureHandle, SamplerHandle) override;
        void BindVertexSampler(CommandBufferHandle, uint32_t, TextureHandle, SamplerHandle) override;

        void DrawPrimitives(CommandBufferHandle, uint32_t, uint32_t, uint32_t, uint32_t) override;
        void DrawIndexed(CommandBufferHandle, uint32_t, uint32_t, uint32_t, int32_t, uint32_t) override;

        void BeginComputePass(CommandBufferHandle, const ComputePassDesc&) override;
        void EndComputePass(CommandBufferHandle) override;
        void BindComputePipeline(CommandBufferHandle, ComputePipelineHandle) override;
        void BindComputeStorageBuffer(CommandBufferHandle, uint32_t, BufferHandle) override;
        void BindComputeStorageTexture(CommandBufferHandle, uint32_t, TextureHandle) override;
        void PushComputeUniformData(CommandBufferHandle, uint32_t, const void*, uint32_t) override;
        void DispatchCompute(CommandBufferHandle, uint32_t, uint32_t, uint32_t) override;

        void WaitForIdle() override;
        void PushDebugGroup(CommandBufferHandle, const char*) override;
        void PopDebugGroup(CommandBufferHandle) override;
        void InsertDebugLabel(CommandBufferHandle, const char*) override;

    private:
        RHIDeviceVulkan() = delete;

        static constexpr uint32_t MaxFramesInFlight  = 2;
        static constexpr uint64_t SwapchainTextureId = 1;

        struct QueueFamilyIndices {
            uint32_t graphics = UINT32_MAX;
            uint32_t present  = UINT32_MAX;

            bool Complete() const {
                return graphics != UINT32_MAX && present != UINT32_MAX;
            }
        };


        struct BufferState {
            VkBuffer buffer                        = VK_NULL_HANDLE;
            VkDeviceMemory memory                  = VK_NULL_HANDLE;
            VkDeviceSize size                      = 0;
            VkMemoryPropertyFlags memoryProperties = 0;
        };

        struct TextureState {
            VkImage image             = VK_NULL_HANDLE;
            VkDeviceMemory memory     = VK_NULL_HANDLE;
            VkImageView view          = VK_NULL_HANDLE;
            VkFormat format           = VK_FORMAT_UNDEFINED;
            TextureFormat rhiFormat   = TextureFormat::Invalid;
            VkImageLayout layout      = VK_IMAGE_LAYOUT_UNDEFINED;
            VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
            TextureType type          = TextureType::Texture2D;
            TextureUsage usage        = TextureUsage::Sampler;
            uint32_t width            = 1;
            uint32_t height           = 1;
            uint32_t layers           = 1;
            uint32_t mipLevels        = 1;
            SampleCount sampleCount   = SampleCount::Count1;
        };

        struct SamplerState {
            VkSampler sampler = VK_NULL_HANDLE;
        };

        struct ShaderState {
            VkShaderModule module       = VK_NULL_HANDLE;
            ShaderStage stage           = ShaderStage::Vertex;
            std::string entryPoint      = "main";
            uint32_t numSamplers        = 0;
            uint32_t numUniformBuffers  = 0;
            uint32_t numStorageTextures = 0;
            uint32_t numStorageBuffers  = 0;
        };

        struct PipelineState {
            VkPipeline pipeline     = VK_NULL_HANDLE;
            VkPipelineLayout layout = VK_NULL_HANDLE;
            std::array<VkDescriptorSetLayout, 3> setLayouts{};
        };

        struct FrameContext {
            VkCommandPool commandPool       = VK_NULL_HANDLE;
            VkCommandBuffer commandBuffer   = VK_NULL_HANDLE;
            VkSemaphore imageAvailable      = VK_NULL_HANDLE;
            VkSemaphore renderFinished      = VK_NULL_HANDLE;
            VkFence fence                   = VK_NULL_HANDLE;
            VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
            std::vector<BufferHandle> transientBuffers;
        };

        struct CommandState {
            VkCommandBuffer buffer   = VK_NULL_HANDLE;
            uint32_t frameIndex      = 0;
            bool rendering           = false;
            bool compute             = false;
            bool acquired            = false;
            VkPipelineLayout layout  = VK_NULL_HANDLE;
            uint64_t boundPipelineId = 0;
            std::unordered_map<VkDescriptorSetLayout, VkDescriptorSet> descriptorSets;
            std::vector<TextureHandle> activeColorTargets;
            TextureHandle activeDepthTarget{};
            std::array<VkDescriptorSet, 3> pendingSets{};
            std::array<bool, 3> setDirty{};
        };

        bool Initialize(Window* window, bool debug);
        bool CreateInstance(bool debug);
        bool CreateDevice();
        bool CreateSwapchain();
        bool CreateFrameContexts();
        bool RecreateSwapchain();

        void DestroySwapchain();
        void DestroyFrameContexts();

        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
        bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;
        bool IsDeviceSuitable(VkPhysicalDevice device) const;
        bool CheckValidationLayerSupport() const;

        uint32_t FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties) const;

        bool CreateImage(const TextureDesc& desc, TextureState& state);
        bool CreateImageView(TextureState& state);
        bool CreateBufferInternal(const BufferDesc& desc, BufferState& state);
        bool CreateStagingBuffer(VkDeviceSize size, BufferState& state);
        BufferHandle AcquireTransientUniformBuffer(uint32_t size);

        void DestroyBufferState(BufferState& state);
        void DestroyTextureState(TextureState& state);

        bool BeginImmediate(VkCommandBuffer& command);
        bool EndImmediate(VkCommandBuffer command);

        void TransitionImage(VkCommandBuffer command,
                             TextureState& texture,
                             VkImageLayout newLayout,
                             uint32_t baseMip    = 0,
                             uint32_t mipCount   = VK_REMAINING_MIP_LEVELS,
                             uint32_t baseLayer  = 0,
                             uint32_t layerCount = VK_REMAINING_ARRAY_LAYERS);

        void TransitionSwapchainImage(VkCommandBuffer command, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

        VkDescriptorSet GetOrCreateDescriptorSet(CommandState& command, uint32_t set);
        VkDescriptorSetLayout CreateDescriptorSetLayout(uint32_t numUniformBuffers,
                                                        uint32_t numSamplers,
                                                        uint32_t numStorageTextures,
                                                        uint32_t numStorageBuffers,
                                                        VkShaderStageFlags stages);

        bool CreatePipelineLayout(std::array<VkDescriptorSetLayout, 3>& layouts, VkPipelineLayout& layout);

        void WriteBufferDescriptor(CommandState& command, uint32_t set, uint32_t binding, BufferHandle buffer, VkDescriptorType type);
        void WriteTextureDescriptor(
            CommandState& command, uint32_t set, uint32_t binding, TextureHandle texture, SamplerHandle sampler, VkDescriptorType type);
        void FlushDescriptorSets(CommandState& command);

        TextureState* GetTexture(TextureHandle handle);
        const TextureState* GetTexture(TextureHandle handle) const;
        BufferState* GetBuffer(BufferHandle handle);
        const BufferState* GetBuffer(BufferHandle handle) const;
        SamplerState* GetSampler(SamplerHandle handle);
        PipelineState* GetGraphicsPipeline(GraphicsPipelineHandle handle);
        PipelineState* GetComputePipeline(ComputePipelineHandle handle);
        CommandState* GetCommand(CommandBufferHandle handle);

        void SetObjectName(VkObjectType type, uint64_t object, const char* name);

        uint64_t NextId() {
            return mNextId++;
        }

        static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                            VkDebugUtilsMessageTypeFlagsEXT type,
                                                            const VkDebugUtilsMessengerCallbackDataEXT* callback,
                                                            void* userData);

    private:
        uint64_t mNextId = 2;

        RHIDeviceInfo mInfo{};
        RHICapabilities mCapabilities{};

        VkInstance mInstance                     = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT mDebugMessenger = VK_NULL_HANDLE;
        VkPhysicalDevice mPhysicalDevice         = VK_NULL_HANDLE;
        VkDevice mDevice                         = VK_NULL_HANDLE;

        QueueFamilyIndices mQueueFamilies{};
        VkQueue mGraphicsQueue = VK_NULL_HANDLE;
        VkQueue mPresentQueue  = VK_NULL_HANDLE;

        VkSurfaceKHR mSurface                = VK_NULL_HANDLE;
        VkSwapchainKHR mSwapchain            = VK_NULL_HANDLE;
        VkFormat mSwapchainFormat            = VK_FORMAT_B8G8R8A8_UNORM;
        VkColorSpaceKHR mSwapchainColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        VkExtent2D mSwapchainExtent{0, 0};

        std::vector<VkImage> mSwapchainImages;
        std::vector<VkImageView> mSwapchainImageViews;
        std::vector<VkImageLayout> mSwapchainLayouts;
        std::vector<VkFence> mImagesInFlight;

        std::array<FrameContext, MaxFramesInFlight> mFrames{};
        uint32_t mCurrentFrame = 0;
        uint32_t mCurrentImage = 0;

        std::unordered_map<uint64_t, CommandState> mCommands;
        std::unordered_map<uint64_t, BufferState> mBuffers;
        std::unordered_map<uint32_t, std::vector<BufferHandle>> mReusableUniformBuffers;
        std::unordered_map<uint64_t, TextureState> mTextures;
        std::unordered_map<uint64_t, SamplerState> mSamplers;
        std::unordered_map<uint64_t, ShaderState> mShaders;
        std::unordered_map<uint64_t, PipelineState> mGraphicsPipelines;
        std::unordered_map<uint64_t, PipelineState> mComputePipelines;

        PFN_vkSetDebugUtilsObjectNameEXT mSetDebugUtilsObjectName   = nullptr;
        PFN_vkCmdBeginDebugUtilsLabelEXT mCmdBeginDebugUtilsLabel   = nullptr;
        PFN_vkCmdEndDebugUtilsLabelEXT mCmdEndDebugUtilsLabel       = nullptr;
        PFN_vkCmdInsertDebugUtilsLabelEXT mCmdInsertDebugUtilsLabel = nullptr;
    };

} // namespace golias
