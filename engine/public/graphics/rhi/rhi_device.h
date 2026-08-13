#pragma once

#include "graphics/rhi/rhi_types.h"
#include "stdafx.h"

namespace golias {

    class Window;

    class RHIDevice {
    public:
    
        // Create an RHI device for the specified window.
        // The debug parameter enables validation layers and debug output.
        // This function throws an error if the device cannot be created.
        RHIDevice(Window* window, bool debug = true) : mWindow(window), mDebugEnabled(debug) {
        }

        virtual ~RHIDevice() = default;

        virtual RHIBackend GetBackend() const = 0;

        virtual RHIDeviceInfo GetDeviceInfo() const = 0;

        virtual RHICapabilities GetCapabilities() const = 0;

        virtual const char* GetDriverName() const = 0;

        virtual void SetVsyncEnabled(bool enabled) = 0;

        virtual bool IsVsyncEnabled() const = 0;

        virtual TextureHandle CreateTexture(const TextureDesc& desc) = 0;

        virtual void DestroyTexture(TextureHandle texture) = 0;

        virtual BufferHandle CreateBuffer(const BufferDesc& desc) = 0;

        virtual void DestroyBuffer(BufferHandle buffer) = 0;

        virtual SamplerHandle CreateSampler(const SamplerDesc& desc) = 0;

        virtual void DestroySampler(SamplerHandle sampler) = 0;

        virtual ShaderHandle CreateShader(const ShaderDesc& desc) = 0;

        virtual void DestroyShader(ShaderHandle shader) = 0;

        virtual GraphicsPipelineHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) = 0;

        virtual void DestroyGraphicsPipeline(GraphicsPipelineHandle pipeline) = 0;

        virtual ComputePipelineHandle CreateComputePipeline(const ComputePipelineDesc& desc) = 0;

        virtual void DestroyComputePipeline(ComputePipelineHandle pipeline) = 0;

        virtual void UploadToBuffer(BufferHandle dst, const void* data, uint32_t size, uint32_t offset = 0) = 0;

        virtual void UploadToTexture(TextureHandle dst, const void* data, uint32_t width, uint32_t height, uint32_t bytesPerPixel = 4) = 0;

        virtual void UploadToTextureLayer(
            TextureHandle dst, const void* data, uint32_t width, uint32_t height, uint32_t layer, uint32_t bytesPerPixel = 4) = 0;

        virtual void GenerateMipmaps(TextureHandle texture) = 0;

        virtual CommandBufferHandle BeginCommandBuffer() = 0;

        virtual void SubmitCommandBuffer(CommandBufferHandle commandBuffer) = 0;

        virtual TextureFormat GetDepthFormat() const {
            return TextureFormat::D32_FLOAT;
        }

        virtual TextureFormat GetSwapchainFormat() const = 0;

        virtual bool
            AcquireSwapchainTexture(CommandBufferHandle commandBuffer, TextureHandle* outTexture, uint32_t* width, uint32_t* height) = 0;

        virtual void
            UpdateBuffer(CommandBufferHandle commandBuffer, const void* data, uint32_t size, BufferHandle dst, uint32_t offset = 0) = 0;

        virtual void BeginRenderPass(CommandBufferHandle commandBuffer, const RenderPassDesc& desc) = 0;

        virtual void EndRenderPass(CommandBufferHandle commandBuffer) = 0;

        virtual void BindGraphicsPipeline(CommandBufferHandle commandBuffer, GraphicsPipelineHandle pipeline) = 0;

        virtual void SetViewport(
            CommandBufferHandle commandBuffer, float x, float y, float width, float height, float minDepth = 0, float maxDepth = 1) = 0;

        virtual void SetScissor(CommandBufferHandle commandBuffer, int x, int y, int width, int height) = 0;

        virtual void BindVertexBuffer(CommandBufferHandle commandBuffer, uint32_t slot, BufferHandle buffer, uint32_t offset = 0) = 0;

        virtual void BindIndexBuffer(CommandBufferHandle commandBuffer, BufferHandle buffer, IndexFormat format, uint32_t offset = 0) = 0;

        virtual void BindUniformBuffer(CommandBufferHandle commandBuffer, uint32_t set, uint32_t binding, BufferHandle buffer) = 0;

        virtual void PushVertexUniformData(CommandBufferHandle commandBuffer, uint32_t slot, const void* data, uint32_t size) = 0;

        virtual void PushFragmentUniformData(CommandBufferHandle commandBuffer, uint32_t slot, const void* data, uint32_t size) = 0;

        virtual void BlitTexture(
            CommandBufferHandle commandBuffer, TextureHandle source, TextureHandle destination, uint32_t width, uint32_t height) {

            LOG_WARN("BlitTexture is not implemented for this RHI backend.");
        }

        virtual void
            BindFragmentSampler(CommandBufferHandle commandBuffer, uint32_t slot, TextureHandle texture, SamplerHandle sampler) = 0;

        virtual void BindVertexSampler(CommandBufferHandle commandBuffer, uint32_t slot, TextureHandle texture, SamplerHandle sampler) = 0;

        virtual void DrawPrimitives(CommandBufferHandle commandBuffer,
                                    uint32_t vertexCount,
                                    uint32_t instanceCount = 1,
                                    uint32_t firstVertex   = 0,
                                    uint32_t firstInstance = 0) = 0;

        virtual void DrawIndexed(CommandBufferHandle commandBuffer,
                                 uint32_t indexCount,
                                 uint32_t instanceCount = 1,
                                 uint32_t firstIndex    = 0,
                                 int32_t vertexOffset   = 0,
                                 uint32_t firstInstance = 0) = 0;

        virtual void BeginComputePass(CommandBufferHandle commandBuffer, const ComputePassDesc& desc = {}) = 0;

        virtual void EndComputePass(CommandBufferHandle commandBuffer) = 0;

        virtual void BindComputePipeline(CommandBufferHandle commandBuffer, ComputePipelineHandle pipeline) = 0;

        virtual void BindComputeStorageBuffer(CommandBufferHandle commandBuffer, uint32_t slot, BufferHandle buffer) = 0;

        virtual void BindComputeStorageTexture(CommandBufferHandle commandBuffer, uint32_t slot, TextureHandle texture) = 0;

        virtual void PushComputeUniformData(CommandBufferHandle commandBuffer, uint32_t slot, const void* data, uint32_t size) = 0;

        virtual void DispatchCompute(CommandBufferHandle commandBuffer, uint32_t x, uint32_t y, uint32_t z) = 0;

        virtual void WaitForIdle() = 0;

        virtual void PushDebugGroup(CommandBufferHandle commandBuffer, const char* name) = 0;

        virtual void PopDebugGroup(CommandBufferHandle commandBuffer) = 0;

        virtual void InsertDebugLabel(CommandBufferHandle commandBuffer, const char* name) = 0;

    protected:
        bool mVsyncEnabled = true;
        bool mDebugEnabled = true;
        Window* mWindow    = nullptr;
    };

} // namespace golias
