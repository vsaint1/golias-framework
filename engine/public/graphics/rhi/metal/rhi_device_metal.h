#pragma once

#include "graphics/rhi/rhi_device.h"

namespace golias {

    class RHIDeviceMetal final : public RHIDevice {
    public:
        RHIDeviceMetal(Window* window, bool debug = true);
        ~RHIDeviceMetal() override;

        RHIBackend GetBackend() const override {
            return RHIBackend::Metal;
        }

        RHIDeviceInfo GetDeviceInfo() const override {
            return mInfo;
        }

        RHICapabilities GetCapabilities() const override {
            return mCapabilities;
        }
        
        const char* GetDriverName() const override {
            return "Metal";
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
            return TextureFormat::B8G8R8A8_UNORM;
        }

        bool AcquireSwapchainTexture(CommandBufferHandle, TextureHandle*, uint32_t*, uint32_t*) override;
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

        RHIDeviceMetal() = delete;

        struct BufferState {
            void* object  = nullptr;
            uint32_t size = 0;
        };

        struct TextureState {
            void* object         = nullptr;
            TextureFormat format = TextureFormat::Invalid;
            uint32_t width = 1, height = 1, layers = 1, mipLevels = 1;
        };

        struct SamplerState {
            void* object = nullptr;
        };

        struct ShaderState {
            void* library        = nullptr;
            void* function       = nullptr;
            ShaderStage stage    = ShaderStage::Vertex;
            uint32_t numSamplers = 0, numUniformBuffers = 0;
        };

        struct PipelineState {
            void* render  = nullptr;
            void* depth   = nullptr;
            void* compute = nullptr;
        };

        struct CommandState {
            void* commandBuffer     = nullptr;
            void* renderEncoder     = nullptr;
            void* computeEncoder    = nullptr;
            void* drawable          = nullptr;
            void* indexBuffer       = nullptr;
            uint32_t indexOffset    = 0;
            IndexFormat indexFormat = IndexFormat::UInt32;
            bool acquired           = false;
        };

        uint64_t NextId() {
            return mNextId++;
        }
        
        BufferState* GetBuffer(BufferHandle);
        TextureState* GetTexture(TextureHandle);
        SamplerState* GetSampler(SamplerHandle);
        CommandState* GetCommand(CommandBufferHandle);
        PipelineState* GetGraphicsPipeline(GraphicsPipelineHandle);
        PipelineState* GetComputePipeline(ComputePipelineHandle);

        uint64_t mNextId = 2;
        RHIDeviceInfo mInfo{};
        RHICapabilities mCapabilities{};

        void* mDevice = nullptr;
        void* mQueue  = nullptr;
        void* mLayer  = nullptr;

        std::unordered_map<uint64_t, BufferState> mBuffers;
        std::unordered_map<uint64_t, TextureState> mTextures;
        std::unordered_map<uint64_t, SamplerState> mSamplers;
        std::unordered_map<uint64_t, ShaderState> mShaders;
        std::unordered_map<uint64_t, PipelineState> mGraphicsPipelines;
        std::unordered_map<uint64_t, PipelineState> mComputePipelines;
        std::unordered_map<uint64_t, CommandState> mCommands;
    };

} // namespace golias
