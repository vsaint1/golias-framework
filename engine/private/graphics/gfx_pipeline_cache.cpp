#include "graphics/gfx_pipeline_cache.h"

namespace golias {
    namespace {
        constexpr uint32_t kHashMagic = 0x9e3779b9u;
        
        template <typename T>
        void HashCombine(size_t& seed, T value) {
            seed ^= std::hash<T>{}(value) + kHashMagic + (seed << 6) + (seed >> 2);
        }

        bool Equal(const VertexBufferDesc& a, const VertexBufferDesc& b) {
            return a.slot == b.slot && a.stride == b.stride && a.stepRate == b.stepRate && a.instanced == b.instanced;
        }

        bool Equal(const VertexAttribute& a, const VertexAttribute& b) {
            return a.location == b.location && a.bufferSlot == b.bufferSlot && a.offset == b.offset && a.format == b.format;
        }
    } // namespace


     GraphicsPipelineStateCache::GraphicsPipelineStateCache(const Ref<RHIDevice>& device) : mDevice(device) {
    }


    bool PipelineKey::operator==(const PipelineKey& other) const {

        if (VertexShader != other.VertexShader || FragmentShader != other.FragmentShader || Rasterizer.fillMode != other.Rasterizer.fillMode
            || Rasterizer.cullMode != other.Rasterizer.cullMode || Rasterizer.frontFace != other.Rasterizer.frontFace
            || Rasterizer.enableDepthBias != other.Rasterizer.enableDepthBias
            || Rasterizer.depthBiasConstantFactor != other.Rasterizer.depthBiasConstantFactor
            || Rasterizer.depthBiasSlopeFactor != other.Rasterizer.depthBiasSlopeFactor
            || Rasterizer.depthBiasClamp != other.Rasterizer.depthBiasClamp
            || DepthStencil.enableDepthTest != other.DepthStencil.enableDepthTest
            || DepthStencil.enableDepthWrite != other.DepthStencil.enableDepthWrite
            || DepthStencil.depthCompareOp != other.DepthStencil.depthCompareOp
            || DepthStencil.enableStencilTest != other.DepthStencil.enableStencilTest
            || BlendState.enableBlend != other.BlendState.enableBlend || BlendState.srcColorFactor != other.BlendState.srcColorFactor
            || BlendState.dstColorFactor != other.BlendState.dstColorFactor || BlendState.colorOp != other.BlendState.colorOp
            || BlendState.srcAlphaFactor != other.BlendState.srcAlphaFactor || BlendState.dstAlphaFactor != other.BlendState.dstAlphaFactor
            || BlendState.alphaOp != other.BlendState.alphaOp || BlendState.writeMask != other.BlendState.writeMask
            || ColorFormat != other.ColorFormat || DepthFormat != other.DepthFormat || Primitive != other.Primitive
            || VertexBuffers.size() != other.VertexBuffers.size() || VertexAttributes.size() != other.VertexAttributes.size()) {
            return false;
        }

        for (size_t i = 0; i < VertexBuffers.size(); ++i) {
            if (!Equal(VertexBuffers[i], other.VertexBuffers[i])) {
                return false;
            }
        }

        for (size_t i = 0; i < VertexAttributes.size(); ++i) {
            if (!Equal(VertexAttributes[i], other.VertexAttributes[i])) {
                return false;
            }
        }

        return true;
    }

    size_t GraphicsPipelineStateCache::KeyHash::operator()(const PipelineKey& key) const {
        size_t seed = key.VertexShader.id ^ (key.FragmentShader.id << 1);

        HashCombine(seed, static_cast<int>(key.ColorFormat));
        HashCombine(seed, static_cast<int>(key.DepthFormat));
        HashCombine(seed, static_cast<int>(key.Primitive));
        HashCombine(seed, static_cast<int>(key.Rasterizer.fillMode));
        HashCombine(seed, static_cast<int>(key.Rasterizer.cullMode));
        HashCombine(seed, static_cast<int>(key.Rasterizer.frontFace));
        HashCombine(seed, key.Rasterizer.enableDepthBias);
        HashCombine(seed, key.Rasterizer.depthBiasConstantFactor);
        HashCombine(seed, key.Rasterizer.depthBiasSlopeFactor);
        HashCombine(seed, key.Rasterizer.depthBiasClamp);
        HashCombine(seed, key.DepthStencil.enableDepthTest);
        HashCombine(seed, key.DepthStencil.enableDepthWrite);
        HashCombine(seed, static_cast<int>(key.DepthStencil.depthCompareOp));
        HashCombine(seed, key.DepthStencil.enableStencilTest);
        HashCombine(seed, key.BlendState.enableBlend);
        HashCombine(seed, static_cast<int>(key.BlendState.srcColorFactor));
        HashCombine(seed, static_cast<int>(key.BlendState.dstColorFactor));
        HashCombine(seed, static_cast<int>(key.BlendState.colorOp));
        HashCombine(seed, static_cast<int>(key.BlendState.srcAlphaFactor));
        HashCombine(seed, static_cast<int>(key.BlendState.dstAlphaFactor));
        HashCombine(seed, static_cast<int>(key.BlendState.alphaOp));
        HashCombine(seed, static_cast<int>(key.BlendState.writeMask));

        for (const auto& buffer : key.VertexBuffers) {
            HashCombine(seed, buffer.slot);
            HashCombine(seed, buffer.stride);
            HashCombine(seed, buffer.stepRate);
            HashCombine(seed, buffer.instanced);
        }

        for (const auto& attribute : key.VertexAttributes) {
            HashCombine(seed, attribute.location);
            HashCombine(seed, attribute.bufferSlot);
            HashCombine(seed, attribute.offset);
            HashCombine(seed, static_cast<int>(attribute.format));
        }

        return seed;
    }

    GraphicsPipelineHandle GraphicsPipelineStateCache::TryGet(const PipelineKey& key) {
        auto it = mPipelines.find(key);
        if (it != mPipelines.end()) {
            return it->second;
        }

        ColorTargetDesc target = {.format = key.ColorFormat, .blendState = key.BlendState};

        VertexInputState vertexInput = {
            .bufferDescs   = key.VertexBuffers.data(),
            .numBuffers    = static_cast<uint32_t>(key.VertexBuffers.size()),
            .attributes    = key.VertexAttributes.data(),
            .numAttributes = static_cast<uint32_t>(key.VertexAttributes.size()),
        };

        GraphicsPipelineDesc desc = {
            .vertexShader      = key.VertexShader,
            .fragmentShader    = key.FragmentShader,
            .vertexInput       = vertexInput,
            .primitiveType     = key.Primitive,
            .rasterizerState   = key.Rasterizer,
            .depthStencilState = key.DepthStencil,
        };


        GraphicsPipelineTargetInfo targetInfo = {.colorTargets    = &target,
                                                 .numColorTargets = 1,
                                                 .hasDepthTarget  = key.DepthFormat != TextureFormat::Invalid,
                                                 .depthFormat     = key.DepthFormat};

        desc.targetInfo = targetInfo;

        const auto pipeline = mDevice->CreateGraphicsPipeline(desc);
        mPipelines.emplace(key, pipeline);
        return pipeline;
    }

    void GraphicsPipelineStateCache::Clear() {
        for (const auto& entry : mPipelines) {
            mDevice->DestroyGraphicsPipeline(entry.second);
        }

        mPipelines.clear();
    }
} // namespace golias
