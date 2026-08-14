#pragma once

#include <cstdint>
#include <string>

namespace golias {

    enum class RHIBackend : uint8_t {
        Compatibility, // OpenGL 3.3 Core  | OpenGL ES 3.0
        Vulkan, // Vulkan 1.3.x
        Metal, // Metal 1.0+
        DirectX12, // DirectX 12
        Auto // Automatically select the best available backend for the platform
    };

#define GOLIAS_DEFINE_HANDLE(Name)                           \
    struct Name {                                            \
        uint64_t id = 0;                                     \
        constexpr explicit operator bool() const {           \
            return id != 0;                                  \
        }                                                    \
        constexpr bool operator==(const Name& other) const { \
            return id == other.id;                           \
        }                                                    \
        constexpr bool operator!=(const Name& other) const { \
            return id != other.id;                           \
        }                                                    \
        bool IsValid() const {                               \
            return id != 0;                                  \
        }                                                    \
    }

    GOLIAS_DEFINE_HANDLE(TextureHandle);
    GOLIAS_DEFINE_HANDLE(BufferHandle);
    GOLIAS_DEFINE_HANDLE(SamplerHandle);
    GOLIAS_DEFINE_HANDLE(ShaderHandle);
    GOLIAS_DEFINE_HANDLE(GraphicsPipelineHandle);
    GOLIAS_DEFINE_HANDLE(ComputePipelineHandle);
    GOLIAS_DEFINE_HANDLE(CommandBufferHandle);

#undef GOLIAS_DEFINE_HANDLE

    enum class ShaderStage : uint8_t { Vertex, Fragment, Compute };

    enum class TextureFormat : uint8_t {
        R8G8B8A8_UNORM,
        R8G8B8A8_SRGB,
        B8G8R8A8_UNORM,
        B8G8R8A8_SRGB,
        R8_UNORM,
        R16G16B16A16_FLOAT,
        R32G32B32A32_FLOAT,
        D24_UNORM_S8_UINT,
        D32_FLOAT,
        D32_FLOAT_S8_UINT,
        Swapchain,
        Invalid
    };

    enum class TextureType : uint8_t { Texture2D, TextureCube, Texture2DArray };

    enum class TextureUsage : uint32_t {
        Sampler      = 1 << 0,
        ColorTarget  = 1 << 1,
        DepthTarget  = 1 << 2,
        StorageRead  = 1 << 3,
        StorageWrite = 1 << 4,
        ComputeWrite = 1 << 5
    };

    inline constexpr TextureUsage operator|(TextureUsage a, TextureUsage b) {
        return static_cast<TextureUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline constexpr bool HasFlag(TextureUsage value, TextureUsage flag) {
        return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) != 0;
    }

    enum class BufferUsage : uint32_t { Vertex = 1 << 0, Index = 1 << 1, Uniform = 1 << 2, Storage = 1 << 3, Indirect = 1 << 4 };

    inline constexpr BufferUsage operator|(BufferUsage a, BufferUsage b) {
        return static_cast<BufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline constexpr bool HasFlag(BufferUsage value, BufferUsage flag) {
        return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) != 0;
    }

    enum class VertexElementFormat : uint8_t { Float, Float2, Float3, Float4, Int, Int2, Int3, Int4, UByte4_Norm };

    enum class IndexFormat : uint8_t { UInt16, UInt32 };

    enum class PrimitiveType : uint8_t { TriangleList, TriangleStrip, LineList, LineStrip, PointList };

    enum class FillMode : uint8_t { Fill, Line };

    enum class CullMode : uint8_t { None, Front, Back };

    enum class FrontFace : uint8_t { CounterClockwise, Clockwise };

    enum class BlendFactor : uint8_t {
        Zero,
        One,
        SrcColor,
        OneMinusSrcColor,
        SrcAlpha,
        OneMinusSrcAlpha,
        DstColor,
        OneMinusDstColor,
        DstAlpha,
        OneMinusDstAlpha
    };

    enum class BlendOp : uint8_t { Add, Subtract, ReverseSubtract, Min, Max };

    enum class ColorComponent : uint8_t { R = 1, G = 2, B = 4, A = 8, All = 15 };

    enum class CompareOp : uint8_t { Never, Less, Equal, LessOrEqual, Greater, NotEqual, GreaterOrEqual, Always };

    enum class Filter : uint8_t { Nearest, Linear };

    enum class SamplerMipmapMode : uint8_t { Nearest, Linear };

    enum class SamplerAddressMode : uint8_t { Repeat, MirroredRepeat, ClampToEdge };

    enum class SampleCount : uint8_t { Count1 = 1, Count2 = 2, Count4 = 4, Count8 = 8 };

    enum class LoadOp : uint8_t { Load, Clear, DontCare };

    enum class StoreOp : uint8_t { Store, DontCare };

    struct TextureDesc {
        TextureType type     = TextureType::Texture2D;
        TextureFormat format = TextureFormat::R8G8B8A8_UNORM;
        TextureUsage usage   = TextureUsage::Sampler;

        uint32_t width         = 1;
        uint32_t height        = 1;
        uint32_t depthOrLayers = 1;
        uint32_t mipLevels     = 1;

        SampleCount sampleCount = SampleCount::Count1;

        float clearColor[4] = {0, 0, 0, 1};

        float clearDepth     = 1.0f;
        uint8_t clearStencil = 0;
    };

    struct BufferDesc {
        BufferUsage usage = BufferUsage::Vertex;
        uint32_t size     = 0;
    };

    struct SamplerDesc {
        Filter minFilter = Filter::Linear;
        Filter magFilter = Filter::Linear;

        SamplerMipmapMode mipmapMode = SamplerMipmapMode::Linear;

        SamplerAddressMode addressU = SamplerAddressMode::ClampToEdge;
        SamplerAddressMode addressV = SamplerAddressMode::ClampToEdge;
        SamplerAddressMode addressW = SamplerAddressMode::ClampToEdge;

        float maxAnisotropy   = 1.0f;
        bool enableAnisotropy = false;
    };

    struct ShaderDesc {
        ShaderStage stage = ShaderStage::Vertex;

        const uint8_t* code = nullptr;
        size_t size         = 0;

        const char* entrypoint = "main";

        uint32_t numSamplers        = 0;
        uint32_t numUniformBuffers  = 0;
        uint32_t numStorageTextures = 0;
        uint32_t numStorageBuffers  = 0;
    };

    struct VertexAttribute {
        uint32_t location   = 0;
        uint32_t bufferSlot = 0;
        uint32_t offset     = 0;

        VertexElementFormat format = VertexElementFormat::Float2;
    };

    struct VertexBufferDesc {
        uint32_t slot     = 0;
        uint32_t stride   = 0;
        uint32_t stepRate = 0;

        bool instanced = false;
    };

    struct VertexInputState {
        const VertexBufferDesc* bufferDescs = nullptr;
        uint32_t numBuffers                 = 0;

        const VertexAttribute* attributes = nullptr;
        uint32_t numAttributes            = 0;
    };

    struct RasterizerState {
        FillMode fillMode   = FillMode::Fill;
        CullMode cullMode   = CullMode::None;
        FrontFace frontFace = FrontFace::CounterClockwise;

        bool enableDepthBias = false;

        float depthBiasConstantFactor = 0;
        float depthBiasSlopeFactor    = 0;
        float depthBiasClamp          = 0;
    };

    struct DepthStencilState {
        bool enableDepthTest  = false;
        bool enableDepthWrite = false;

        CompareOp depthCompareOp = CompareOp::Less;

        bool enableStencilTest = false;
    };

    struct ColorTargetBlendState {
        bool enableBlend = false;

        BlendFactor srcColorFactor = BlendFactor::One;
        BlendFactor dstColorFactor = BlendFactor::Zero;

        BlendOp colorOp = BlendOp::Add;

        BlendFactor srcAlphaFactor = BlendFactor::One;
        BlendFactor dstAlphaFactor = BlendFactor::Zero;

        BlendOp alphaOp = BlendOp::Add;

        ColorComponent writeMask = ColorComponent::All;
    };

    struct ColorTargetDesc {
        TextureFormat format = TextureFormat::Swapchain;
        ColorTargetBlendState blendState;
    };

    struct GraphicsPipelineTargetInfo {
        const ColorTargetDesc* colorTargets = nullptr;
        uint32_t numColorTargets            = 0;

        bool hasDepthTarget       = false;
        TextureFormat depthFormat = TextureFormat::D24_UNORM_S8_UINT;
    };

    struct GraphicsPipelineDesc {
        ShaderHandle vertexShader;
        ShaderHandle fragmentShader;

        VertexInputState vertexInput;

        PrimitiveType primitiveType = PrimitiveType::TriangleList;

        RasterizerState rasterizerState;
        DepthStencilState depthStencilState;

        SampleCount sampleCount = SampleCount::Count1;

        GraphicsPipelineTargetInfo targetInfo;
    };

    struct ComputePipelineDesc {
        ShaderHandle computeShader;

        const uint8_t* spirvCode = nullptr;
        size_t spirvSize         = 0;

        const char* entrypoint = "main";

        uint32_t numSamplers                 = 0;
        uint32_t numReadOnlyStorageTextures  = 0;
        uint32_t numReadOnlyStorageBuffers   = 0;
        uint32_t numReadWriteStorageTextures = 0;
        uint32_t numReadWriteStorageBuffers  = 0;
        uint32_t numUniformBuffers           = 0;

        uint32_t threadCountX = 8;
        uint32_t threadCountY = 8;
        uint32_t threadCountZ = 1;
    };

    struct ComputePassDesc {
        const TextureHandle* readWriteTextures = nullptr;
        uint32_t numReadWriteTextures          = 0;

        const BufferHandle* readWriteBuffers = nullptr;
        uint32_t numReadWriteBuffers         = 0;
    };

    struct ClearColor {
        float r = 0;
        float g = 0;
        float b = 0;
        float a = 1;
    };

    struct RenderPassColorTarget {
        TextureHandle texture;

        LoadOp loadOp   = LoadOp::Clear;
        StoreOp storeOp = StoreOp::Store;

        ClearColor clearColor;
    };

    struct RenderPassDesc {
        const RenderPassColorTarget* colorTargets = nullptr;
        uint32_t numColorTargets                  = 0;

        TextureHandle depthStencilTarget;

        LoadOp depthLoadOp   = LoadOp::Clear;
        StoreOp depthStoreOp = StoreOp::DontCare;

        float clearDepth = 1.0f;
    };

    struct RHIDeviceInfo {
        std::string deviceName;
        std::string driverName;
        std::string apiVersion;

        RHIBackend backend = RHIBackend::Auto;

        uint32_t vendorId = 0;
        uint32_t deviceId = 0;
    };

    struct RHICapabilities {
        bool supportsCompute        = false;
        bool supportsStorageBuffers = false;
        bool supportsMultisampling  = false;
        bool supportsAnisotropy     = false;
        bool supportsInstancing     = true;
        bool supportsWireframe      = false;
        bool supportsDebugMarkers   = false;

        SampleCount maxSampleCount = SampleCount::Count1;

        uint32_t maxTextureSize  = 4096;
        uint32_t maxColorTargets = 4;
    };

} // namespace golias
