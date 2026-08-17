#pragma once

#include "graphics/rhi/rhi_device.h"

namespace golias {

    struct PipelineKey {
        ShaderHandle VertexShader;
        ShaderHandle FragmentShader;

        std::vector<VertexBufferDesc> VertexBuffers;
        std::vector<VertexAttribute> VertexAttributes;

        RasterizerState Rasterizer;
        DepthStencilState DepthStencil;
        ColorTargetBlendState BlendState;

        TextureFormat ColorFormat = TextureFormat::Swapchain;
        TextureFormat DepthFormat = TextureFormat::Invalid;

        PrimitiveType Primitive   = PrimitiveType::TriangleList;

        bool operator==(const PipelineKey& other) const;
    };

    // PipelineStateCache is a cache for graphics pipelines.
    class GraphicsPipelineStateCache {
    public:
        explicit GraphicsPipelineStateCache(const Ref<RHIDevice>& device);

        // TryGet returns a cached graphics pipeline or creates a new one if it doesn't exist. Returns an invalid handle if creation fails.
        GraphicsPipelineHandle TryGet(const PipelineKey& key);

        void Clear();

    private:
    
        struct KeyHash {
            size_t operator()(const PipelineKey& key) const;
        };

        Ref<RHIDevice> mDevice = nullptr;
        std::unordered_map<PipelineKey, GraphicsPipelineHandle, KeyHash> mPipelines = {};
    };

} // namespace golias
