#include "graphics/rhi/metal/rhi_device_metal.h"

#include "core/window.h"
#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

namespace golias {
    namespace {
        static id Obj(void* p) {
            return (__bridge id) p;
        }

        static void* Keep(id object) {
            return object ? (__bridge_retained void*) object : nullptr;
        }

        static void Drop(void*& object) {
            if (object) {
                CFBridgingRelease(object);
                object = nullptr;
            }
        }

        static MTLPixelFormat PixelFormat(TextureFormat format) {
            switch (format) {
            case TextureFormat::R8G8B8A8_UNORM:
                return MTLPixelFormatRGBA8Unorm;
            case TextureFormat::R8G8B8A8_SRGB:
                return MTLPixelFormatRGBA8Unorm_sRGB;
            case TextureFormat::B8G8R8A8_UNORM:
                return MTLPixelFormatBGRA8Unorm;
            case TextureFormat::B8G8R8A8_SRGB:
                return MTLPixelFormatBGRA8Unorm_sRGB;
            case TextureFormat::R8_UNORM:
                return MTLPixelFormatR8Unorm;
            case TextureFormat::R16G16B16A16_FLOAT:
                return MTLPixelFormatRGBA16Float;
            case TextureFormat::R32G32B32A32_FLOAT:
                return MTLPixelFormatRGBA32Float;
            case TextureFormat::D24_UNORM_S8_UINT:
                return MTLPixelFormatDepth24Unorm_Stencil8;
            case TextureFormat::D32_FLOAT:
                return MTLPixelFormatDepth32Float;
            case TextureFormat::D32_FLOAT_S8_UINT:
                return MTLPixelFormatDepth32Float_Stencil8;
            case TextureFormat::Swapchain:
                return MTLPixelFormatBGRA8Unorm;
            default:
                return MTLPixelFormatInvalid;
            }
        }

        static MTLCompareFunction Compare(CompareOp op) {
            switch (op) {
            case CompareOp::Never:
                return MTLCompareFunctionNever;
            case CompareOp::Less:
                return MTLCompareFunctionLess;
            case CompareOp::Equal:
                return MTLCompareFunctionEqual;
            case CompareOp::LessOrEqual:
                return MTLCompareFunctionLessEqual;
            case CompareOp::Greater:
                return MTLCompareFunctionGreater;
            case CompareOp::NotEqual:
                return MTLCompareFunctionNotEqual;
            case CompareOp::GreaterOrEqual:
                return MTLCompareFunctionGreaterEqual;
            default:
                return MTLCompareFunctionAlways;
            }
        }

        static MTLPrimitiveType Primitive(PrimitiveType type) {
            switch (type) {
            case PrimitiveType::TriangleStrip:
                return MTLPrimitiveTypeTriangleStrip;
            case PrimitiveType::LineList:
                return MTLPrimitiveTypeLine;
            case PrimitiveType::LineStrip:
                return MTLPrimitiveTypeLineStrip;
            case PrimitiveType::PointList:
                return MTLPrimitiveTypePoint;
            default:
                return MTLPrimitiveTypeTriangle;
            }
        }

        static MTLVertexFormat VertexFormat(VertexElementFormat format) {
            switch (format) {
            case VertexElementFormat::Float:
                return MTLVertexFormatFloat;
            case VertexElementFormat::Float2:
                return MTLVertexFormatFloat2;
            case VertexElementFormat::Float3:
                return MTLVertexFormatFloat3;
            case VertexElementFormat::Float4:
                return MTLVertexFormatFloat4;
            case VertexElementFormat::Int:
                return MTLVertexFormatInt;
            case VertexElementFormat::Int2:
                return MTLVertexFormatInt2;
            case VertexElementFormat::Int3:
                return MTLVertexFormatInt3;
            case VertexElementFormat::Int4:
                return MTLVertexFormatInt4;
            case VertexElementFormat::UByte4_Norm:
                return MTLVertexFormatUChar4Normalized;
            default:
                return MTLVertexFormatInvalid;
            }
        }

        static MTLBlendFactor BlendFactorFor(BlendFactor factor) {
            switch (factor) {
            case BlendFactor::Zero:
                return MTLBlendFactorZero;
            case BlendFactor::SrcColor:
                return MTLBlendFactorSourceColor;
            case BlendFactor::OneMinusSrcColor:
                return MTLBlendFactorOneMinusSourceColor;
            case BlendFactor::SrcAlpha:
                return MTLBlendFactorSourceAlpha;
            case BlendFactor::OneMinusSrcAlpha:
                return MTLBlendFactorOneMinusSourceAlpha;
            case BlendFactor::DstColor:
                return MTLBlendFactorDestinationColor;
            case BlendFactor::OneMinusDstColor:
                return MTLBlendFactorOneMinusDestinationColor;
            case BlendFactor::DstAlpha:
                return MTLBlendFactorDestinationAlpha;
            case BlendFactor::OneMinusDstAlpha:
                return MTLBlendFactorOneMinusDestinationAlpha;
            default:
                return MTLBlendFactorOne;
            }
        }
    } // namespace

    RHIDeviceMetal::RHIDeviceMetal(Window* window, bool debug) : RHIDevice(window, debug) {
        @autoreleasepool {
            id<MTLDevice> device   = MTLCreateSystemDefaultDevice();
            void* nativeViewHandle = window ? window->GetNativeViewHandle() : nullptr;
            if (!device || !nativeViewHandle) {
                LOG_FATAL("Failed to initialize Metal device or Cocoa window.");
            }

            mDevice = Keep(device);
            mQueue  = Keep([device newCommandQueue]);

            NSView* contentView    = (__bridge NSView*) nativeViewHandle;
            NSWindow* nativeWindow = contentView.window;
            if (!nativeWindow) {
                LOG_FATAL("Cocoa view is not attached to a window for Metal layer.");
            }

            CAMetalLayer* layer      = [CAMetalLayer layer];
            layer.device             = device;
            layer.pixelFormat        = MTLPixelFormatBGRA8Unorm;
            layer.framebufferOnly    = NO;
            layer.displaySyncEnabled = mVsyncEnabled;
            layer.contentsScale      = nativeWindow.backingScaleFactor;
            contentView.wantsLayer   = YES;
            contentView.layer        = layer;
            layer.frame              = contentView.bounds;
            layer.autoresizingMask   = kCALayerWidthSizable | kCALayerHeightSizable;
            mLayer                   = Keep(layer);

            mInfo.deviceName                     = device.name.UTF8String ? device.name.UTF8String : "Apple GPU";
            mInfo.driverName                     = "Metal";
            mInfo.apiVersion                     = "Metal 2.0+";
            mInfo.backend                        = RHIBackend::Metal;
            mCapabilities.supportsCompute        = [device supportsFamily:MTLGPUFamilyApple1] || [device supportsFamily:MTLGPUFamilyMac2];
            mCapabilities.supportsStorageBuffers = mCapabilities.supportsCompute;
            mCapabilities.supportsMultisampling  = YES;
            mCapabilities.supportsAnisotropy     = YES;
            mCapabilities.supportsWireframe      = YES;
            mCapabilities.supportsDebugMarkers   = YES;
            mCapabilities.maxSampleCount         = SampleCount::Count8;
            mCapabilities.maxTextureSize         = 16384;
            mCapabilities.maxColorTargets        = 8;
        }
    }

    RHIDeviceMetal::~RHIDeviceMetal() {
        WaitForIdle();
        for (auto& [id, state] : mGraphicsPipelines) {
            Drop(state.render);
            Drop(state.depth);
        }

        for (auto& [id, state] : mComputePipelines) {
            Drop(state.compute);
        }

        for (auto& [id, state] : mShaders) {
            Drop(state.function);
            Drop(state.library);
        }

        for (auto& [id, state] : mSamplers) {
            Drop(state.object);
        }

        for (auto& [id, state] : mTextures) {
            Drop(state.object);
        }

        for (auto& [id, state] : mBuffers) {
            Drop(state.object);
        }

        Drop(mLayer);
        Drop(mQueue);
        Drop(mDevice);
    }

    void RHIDeviceMetal::SetVsyncEnabled(bool enabled) {
        mVsyncEnabled = enabled;
        if (mLayer) {
            ((CAMetalLayer*) Obj(mLayer)).displaySyncEnabled = enabled;
        }
    }

    TextureHandle RHIDeviceMetal::CreateTexture(const TextureDesc& desc) {
        if (desc.format == TextureFormat::Swapchain) {
            return TextureHandle{1};
        }

        MTLPixelFormat format = PixelFormat(desc.format);
        if (format == MTLPixelFormatInvalid || desc.width == 0 || desc.height == 0) {
            return {};
        }

        MTLTextureDescriptor* d = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:format
                                                                                     width:desc.width
                                                                                    height:desc.height
                                                                                 mipmapped:desc.mipLevels > 1];
        d.mipmapLevelCount      = std::max(1u, desc.mipLevels);
        d.arrayLength           = desc.type == TextureType::TextureCube ? 6 : std::max(1u, desc.depthOrLayers);
        d.textureType           = desc.type == TextureType::TextureCube
                                    ? MTLTextureTypeCube
                                    : (desc.type == TextureType::Texture2DArray ? MTLTextureType2DArray : MTLTextureType2D);
        d.sampleCount           = static_cast<NSUInteger>(desc.sampleCount);
        d.usage                 = MTLTextureUsageShaderRead;

        if (HasFlag(desc.usage, TextureUsage::ColorTarget) || HasFlag(desc.usage, TextureUsage::DepthTarget)) {
            d.usage |= MTLTextureUsageRenderTarget;
        }

        if (HasFlag(desc.usage, TextureUsage::StorageRead) || HasFlag(desc.usage, TextureUsage::StorageWrite)
            || HasFlag(desc.usage, TextureUsage::ComputeWrite)) {
            d.usage |= MTLTextureUsageShaderWrite;
        }

        id<MTLTexture> texture = [Obj(mDevice) newTextureWithDescriptor:d];
        if (!texture) {
            return {};
        }

        uint64_t id = NextId();
        mTextures.emplace(id,
                          TextureState{Keep(texture),
                                       desc.format,
                                       desc.width,
                                       desc.height,
                                       static_cast<uint32_t>(d.arrayLength),
                                       static_cast<uint32_t>(d.mipmapLevelCount)});
        return TextureHandle{id};
    }

    void RHIDeviceMetal::DestroyTexture(TextureHandle handle) {
        auto it = mTextures.find(handle.id);
        if (it != mTextures.end()) {
            Drop(it->second.object);
            mTextures.erase(it);
        }
    }

    BufferHandle RHIDeviceMetal::CreateBuffer(const BufferDesc& desc) {
        if (!desc.size) {
            return {};
        }

        id<MTLBuffer> buffer = [Obj(mDevice) newBufferWithLength:desc.size options:MTLResourceStorageModeShared];
        if (!buffer) {
            return {};
        }

        uint64_t id = NextId();
        mBuffers.emplace(id, BufferState{Keep(buffer), desc.size});
        return BufferHandle{id};
    }
    void RHIDeviceMetal::DestroyBuffer(BufferHandle handle) {
        auto it = mBuffers.find(handle.id);
        if (it != mBuffers.end()) {
            Drop(it->second.object);
            mBuffers.erase(it);
        }
    }

    SamplerHandle RHIDeviceMetal::CreateSampler(const SamplerDesc& desc) {
        MTLSamplerDescriptor* d = [MTLSamplerDescriptor new];
        d.minFilter             = desc.minFilter == Filter::Nearest ? MTLSamplerMinMagFilterNearest : MTLSamplerMinMagFilterLinear;
        d.magFilter             = desc.magFilter == Filter::Nearest ? MTLSamplerMinMagFilterNearest : MTLSamplerMinMagFilterLinear;
        d.mipFilter             = desc.mipmapMode == SamplerMipmapMode::Nearest ? MTLSamplerMipFilterNearest : MTLSamplerMipFilterLinear;
        d.sAddressMode          = desc.addressU == SamplerAddressMode::Repeat
                                    ? MTLSamplerAddressModeRepeat
                                    : (desc.addressU == SamplerAddressMode::MirroredRepeat ? MTLSamplerAddressModeMirrorRepeat
                                                                                           : MTLSamplerAddressModeClampToEdge);
        d.tAddressMode          = desc.addressV == SamplerAddressMode::Repeat
                                    ? MTLSamplerAddressModeRepeat
                                    : (desc.addressV == SamplerAddressMode::MirroredRepeat ? MTLSamplerAddressModeMirrorRepeat
                                                                                           : MTLSamplerAddressModeClampToEdge);
        d.rAddressMode          = desc.addressW == SamplerAddressMode::Repeat
                                    ? MTLSamplerAddressModeRepeat
                                    : (desc.addressW == SamplerAddressMode::MirroredRepeat ? MTLSamplerAddressModeMirrorRepeat
                                                                                           : MTLSamplerAddressModeClampToEdge);
        d.maxAnisotropy         = desc.enableAnisotropy ? std::max(1u, static_cast<uint32_t>(desc.maxAnisotropy)) : 1;
        id<MTLSamplerState> sampler = [Obj(mDevice) newSamplerStateWithDescriptor:d];
        if (!sampler) {
            return {};
        }

        uint64_t id = NextId();
        mSamplers.emplace(id, SamplerState{Keep(sampler)});
        return SamplerHandle{id};
    }
    void RHIDeviceMetal::DestroySampler(SamplerHandle handle) {
        auto it = mSamplers.find(handle.id);
        if (it != mSamplers.end()) {
            Drop(it->second.object);
            mSamplers.erase(it);
        }
    }

    ShaderHandle RHIDeviceMetal::CreateShader(const ShaderDesc& desc) {
        if (!desc.code || !desc.size) {
            return {};
        }

        NSData* data           = [NSData dataWithBytes:desc.code length:desc.size];
        NSError* error         = nil;
        id<MTLLibrary> library = nil;

        NSString* source = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
        if (source) {
            library = [Obj(mDevice) newLibraryWithSource:source options:nil error:&error];
        } else {
            dispatch_data_t metalData =
                dispatch_data_create(data.bytes, data.length, dispatch_get_main_queue(), DISPATCH_DATA_DESTRUCTOR_DEFAULT);
            library = [Obj(mDevice) newLibraryWithData:metalData error:&error];
        }

        if (!library) {
            LOG_ERROR("Metal shader compilation failed: {}", error.localizedDescription.UTF8String);
            return {};
        }

        NSString* entry          = [NSString stringWithUTF8String:desc.entrypoint ? desc.entrypoint : "main"];
        id<MTLFunction> function = [library newFunctionWithName:entry];
        if (!function) {
            LOG_ERROR("Metal shader entry point '{}' was not found.", desc.entrypoint);
            return {};
        }

        uint64_t id = NextId();
        mShaders.emplace(id, ShaderState{Keep(library), Keep(function), desc.stage, desc.numSamplers, desc.numUniformBuffers});
        return ShaderHandle{id};
    }
    void RHIDeviceMetal::DestroyShader(ShaderHandle handle) {
        auto it = mShaders.find(handle.id);
        if (it != mShaders.end()) {
            Drop(it->second.function);
            Drop(it->second.library);
            mShaders.erase(it);
        }
    }

    GraphicsPipelineHandle RHIDeviceMetal::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) {
        auto v = mShaders.find(desc.vertexShader.id), f = mShaders.find(desc.fragmentShader.id);
        if (v == mShaders.end() || f == mShaders.end()) {
            return {};
        }

        MTLRenderPipelineDescriptor* p = [MTLRenderPipelineDescriptor new];
        p.vertexFunction               = Obj(v->second.function);
        p.fragmentFunction             = Obj(f->second.function);
        p.vertexDescriptor             = [MTLVertexDescriptor new];
        for (uint32_t i = 0; i < desc.vertexInput.numBuffers; ++i) {
            auto& b                                      = desc.vertexInput.bufferDescs[i];
            const uint32_t metalSlot                     = 10 + b.slot;
            p.vertexDescriptor.layouts[metalSlot].stride = b.stride;
            p.vertexDescriptor.layouts[metalSlot].stepFunction =
                b.instanced ? MTLVertexStepFunctionPerInstance : MTLVertexStepFunctionPerVertex;
            p.vertexDescriptor.layouts[metalSlot].stepRate = std::max(1u, b.stepRate);
        }

        for (uint32_t i = 0; i < desc.vertexInput.numAttributes; ++i) {
            auto& a                                          = desc.vertexInput.attributes[i];
            p.vertexDescriptor.attributes[a.location].format = VertexFormat(a.format);
            p.vertexDescriptor.attributes[a.location].offset = a.offset;
            // Keep vertex streams separate from uniform slots 0-2.
            p.vertexDescriptor.attributes[a.location].bufferIndex = 10 + a.bufferSlot;
        }

        for (uint32_t i = 0; i < desc.targetInfo.numColorTargets; ++i) {
            auto t                      = p.colorAttachments[i];
            t.pixelFormat               = PixelFormat(desc.targetInfo.colorTargets[i].format);
            auto& b                     = desc.targetInfo.colorTargets[i].blendState;
            t.blendingEnabled           = b.enableBlend;
            t.sourceRGBBlendFactor      = BlendFactorFor(b.srcColorFactor);
            t.destinationRGBBlendFactor = BlendFactorFor(b.dstColorFactor);
            t.rgbBlendOperation         = MTLBlendOperationAdd;
        }

        if (desc.targetInfo.hasDepthTarget) {
            p.depthAttachmentPixelFormat = PixelFormat(desc.targetInfo.depthFormat);
        }

        NSError* error                    = nil;
        id<MTLRenderPipelineState> render = [Obj(mDevice) newRenderPipelineStateWithDescriptor:p error:&error];
        if (!render) {
            LOG_ERROR("Metal render pipeline creation failed: {}", error.localizedDescription.UTF8String);
            return {};
        }

        MTLDepthStencilDescriptor* dd = [MTLDepthStencilDescriptor new];
        dd.depthCompareFunction =
            desc.depthStencilState.enableDepthTest ? Compare(desc.depthStencilState.depthCompareOp) : MTLCompareFunctionAlways;
        dd.depthWriteEnabled = desc.depthStencilState.enableDepthWrite;

        id<MTLDepthStencilState> depth = [Obj(mDevice) newDepthStencilStateWithDescriptor:dd];

        uint64_t id = NextId();
        mGraphicsPipelines.emplace(id, PipelineState{Keep(render), Keep(depth), nullptr});
        return GraphicsPipelineHandle{id};
    }
    void RHIDeviceMetal::DestroyGraphicsPipeline(GraphicsPipelineHandle handle) {
        auto it = mGraphicsPipelines.find(handle.id);
        if (it != mGraphicsPipelines.end()) {
            Drop(it->second.render);
            Drop(it->second.depth);
            mGraphicsPipelines.erase(it);
        }
    }

    ComputePipelineHandle RHIDeviceMetal::CreateComputePipeline(const ComputePipelineDesc& desc) {
        ShaderHandle shader = desc.computeShader;

        LOG_WARN("Metal compute pipeline creation is not fully implemented. This is a placeholder.");

        uint64_t id = NextId();
        mComputePipelines.emplace(id, PipelineState{nullptr, nullptr, nullptr});
        return ComputePipelineHandle{id};
    }
    
    void RHIDeviceMetal::DestroyComputePipeline(ComputePipelineHandle handle) {
        auto it = mComputePipelines.find(handle.id);
        if (it != mComputePipelines.end()) {
            Drop(it->second.compute);
            mComputePipelines.erase(it);
        }
    }

    void RHIDeviceMetal::UploadToBuffer(BufferHandle handle, const void* data, uint32_t size, uint32_t offset) {
        auto* b = GetBuffer(handle);
        if (b && data && offset + size <= b->size) {
            memcpy(static_cast<uint8_t*>([(id<MTLBuffer>) Obj(b->object) contents]) + offset, data, size);
        }
    }

    void RHIDeviceMetal::UploadToTexture(TextureHandle handle, const void* data, uint32_t width, uint32_t height, uint32_t mipLevel) {
        UploadToTextureLayer(handle, data, width, height, 0, mipLevel);
    }

    void RHIDeviceMetal::UploadToTextureLayer(
        TextureHandle handle, const void* data, uint32_t width, uint32_t height, uint32_t layer, uint32_t mipLevel) {
        auto* t = GetTexture(handle);
        if (!t || !data) {
            return;
        }

        MTLRegion r = MTLRegionMake2D(0, 0, width, height);
        [(id<MTLTexture>) Obj(t->object) replaceRegion:r
                                           mipmapLevel:mipLevel
                                                 slice:layer
                                             withBytes:data
                                           bytesPerRow:width * 4
                                         bytesPerImage:width * height * 4];
    }

    void RHIDeviceMetal::GenerateMipmaps(TextureHandle handle) {
        auto* t = GetTexture(handle);
        if (!t) {
            return;
        }

        id<MTLCommandBuffer> cb        = [Obj(mQueue) commandBuffer];
        id<MTLBlitCommandEncoder> blit = [cb blitCommandEncoder];
        [blit generateMipmapsForTexture:Obj(t->object)];
        [blit endEncoding];
        [cb commit];
        [cb waitUntilCompleted];
    }

    CommandBufferHandle RHIDeviceMetal::BeginCommandBuffer() {
        id<MTLCommandBuffer> cb = [Obj(mQueue) commandBuffer];
        uint64_t id             = NextId();
        mCommands.emplace(id, CommandState{Keep(cb)});
        return CommandBufferHandle{id};
    }

    bool RHIDeviceMetal::AcquireSwapchainTexture(CommandBufferHandle handle, TextureHandle* out, uint32_t* width, uint32_t* height) {
        auto* c = GetCommand(handle);
        if (!c) {
            return false;
        }

        CAMetalLayer* layer = (CAMetalLayer*) Obj(mLayer);

        int w, h;
        mWindow->GetFramebufferSize(&w, &h);
        NSView* contentView    = (__bridge NSView*) mWindow->GetNativeViewHandle();
        NSWindow* nativeWindow = contentView.window;
        layer.contentsScale    = nativeWindow.backingScaleFactor;
        layer.frame            = contentView.bounds;
        layer.drawableSize     = CGSizeMake(w, h);

        id<CAMetalDrawable> drawable = [layer nextDrawable];

        if (!drawable) {
            return false;
        }

        c->drawable = Keep(drawable);
        c->acquired = true;
        *out        = TextureHandle{1};
        *width      = drawable.texture.width;
        *height     = drawable.texture.height;
        return true;
    }

    void RHIDeviceMetal::SubmitCommandBuffer(CommandBufferHandle handle) {
        auto it = mCommands.find(handle.id);
        if (it == mCommands.end()) {
            return;
        }

        auto& c = it->second;
        if (c.renderEncoder) {
            [(id<MTLRenderCommandEncoder>) Obj(c.renderEncoder) endEncoding];
            Drop(c.renderEncoder);
        }

        if (c.computeEncoder) {
            [(id<MTLComputeCommandEncoder>) Obj(c.computeEncoder) endEncoding];
            Drop(c.computeEncoder);
        }

        id<MTLCommandBuffer> cb = Obj(c.commandBuffer);
        if (c.drawable) {
            [cb presentDrawable:Obj(c.drawable)];
        }

        [cb addCompletedHandler:^(id<MTLCommandBuffer> completed) {
          if (completed.status == MTLCommandBufferStatusError) {
              LOG_ERROR("Metal command buffer failed: {}", completed.error.localizedDescription.UTF8String);
          }
        }];

        [cb commit];
        Drop(c.drawable);
        Drop(c.commandBuffer);
        mCommands.erase(it);
    }

    void RHIDeviceMetal::UpdateBuffer(CommandBufferHandle, const void* data, uint32_t size, BufferHandle dst, uint32_t offset) {
        UploadToBuffer(dst, data, size, offset);
    }

    void RHIDeviceMetal::BeginRenderPass(CommandBufferHandle handle, const RenderPassDesc& desc) {
        auto* c = GetCommand(handle);
        if (!c) {
            return;
        }

        MTLRenderPassDescriptor* p = [MTLRenderPassDescriptor renderPassDescriptor];
        for (uint32_t i = 0; i < desc.numColorTargets; ++i) {
            auto* t               = GetTexture(desc.colorTargets[i].texture);
            id<MTLTexture> target = t ? Obj(t->object) : (c->drawable ? [(id<CAMetalDrawable>) Obj(c->drawable) texture] : nil);
            if (target) {
                p.colorAttachments[i].texture = target;
            }

            p.colorAttachments[i].loadAction =
                desc.colorTargets[i].loadOp == LoadOp::Clear
                    ? MTLLoadActionClear
                    : (desc.colorTargets[i].loadOp == LoadOp::Load ? MTLLoadActionLoad : MTLLoadActionDontCare);
            auto& x                          = desc.colorTargets[i].clearColor;
            p.colorAttachments[i].clearColor = MTLClearColorMake(x.r, x.g, x.b, x.a);
            p.colorAttachments[i].storeAction =
                desc.colorTargets[i].storeOp == StoreOp::Store ? MTLStoreActionStore : MTLStoreActionDontCare;
        }

        if (desc.depthStencilTarget) {
            auto* t = GetTexture(desc.depthStencilTarget);
            if (t) {
                p.depthAttachment.texture     = Obj(t->object);
                p.depthAttachment.loadAction  = desc.depthLoadOp == LoadOp::Clear ? MTLLoadActionClear : MTLLoadActionLoad;
                p.depthAttachment.clearDepth  = desc.clearDepth;
                p.depthAttachment.storeAction = desc.depthStoreOp == StoreOp::Store ? MTLStoreActionStore : MTLStoreActionDontCare;
            }
        }

        c->renderEncoder = Keep([(id<MTLCommandBuffer>) Obj(c->commandBuffer) renderCommandEncoderWithDescriptor:p]);
    }

    void RHIDeviceMetal::EndRenderPass(CommandBufferHandle handle) {
        auto* c = GetCommand(handle);
        if (c && c->renderEncoder) {
            [(id<MTLRenderCommandEncoder>) Obj(c->renderEncoder) endEncoding];
            Drop(c->renderEncoder);
        }
    }

    void RHIDeviceMetal::BindGraphicsPipeline(CommandBufferHandle h, GraphicsPipelineHandle p) {
        auto* c = GetCommand(h);
        auto it = mGraphicsPipelines.find(p.id);
        if (c && c->renderEncoder && it != mGraphicsPipelines.end()) {
            auto e = (id<MTLRenderCommandEncoder>) Obj(c->renderEncoder);
            [e setRenderPipelineState:Obj(it->second.render)];
            [e setDepthStencilState:Obj(it->second.depth)];
        }
    }

    void RHIDeviceMetal::SetViewport(CommandBufferHandle h, float x, float y, float w, float z, float n, float f) {
        auto* c = GetCommand(h);
        if (c && c->renderEncoder) {
            [(id<MTLRenderCommandEncoder>) Obj(c->renderEncoder) setViewport:(MTLViewport) {x, y, w, z, n, f}];
        }
    }

    void RHIDeviceMetal::SetScissor(CommandBufferHandle h, int x, int y, int w, int z) {
        auto* c = GetCommand(h);
        if (c && c->renderEncoder) {
            [(id<MTLRenderCommandEncoder>) Obj(c->renderEncoder) setScissorRect:(MTLScissorRect) {(NSUInteger) std::max(0, x),
                                                                                                  (NSUInteger) std::max(0, y),
                                                                                                  (NSUInteger) std::max(0, w),
                                                                                                  (NSUInteger) std::max(0, z)}];
        }
    }

    void RHIDeviceMetal::BindVertexBuffer(CommandBufferHandle h, uint32_t slot, BufferHandle b, uint32_t off) {
        auto* c = GetCommand(h);
        auto* x = GetBuffer(b);
        if (c && x && c->renderEncoder) {
            [(id<MTLRenderCommandEncoder>) Obj(c->renderEncoder) setVertexBuffer:Obj(x->object) offset:off atIndex:10 + slot];
        }
    }

    void RHIDeviceMetal::BindIndexBuffer(CommandBufferHandle h, BufferHandle b, IndexFormat format, uint32_t offset) {
        auto* c = GetCommand(h);
        auto* x = GetBuffer(b);
        if (c && x) {
            c->indexBuffer = x->object;
            c->indexOffset = offset;
            c->indexFormat = format;
        }
    }

    void RHIDeviceMetal::BindUniformBuffer(CommandBufferHandle h, uint32_t, uint32_t slot, BufferHandle b) {
        auto* c = GetCommand(h);
        auto* x = GetBuffer(b);
        if (c && x && c->renderEncoder) {
            [(id<MTLRenderCommandEncoder>) Obj(c->renderEncoder) setVertexBuffer:Obj(x->object) offset:0 atIndex:slot];
        }
    }

    void RHIDeviceMetal::PushVertexUniformData(CommandBufferHandle h, uint32_t slot, const void* d, uint32_t n) {
        auto* c = GetCommand(h);
        if (c && c->renderEncoder) {
            [(id<MTLRenderCommandEncoder>) Obj(c->renderEncoder) setVertexBytes:d length:n atIndex:slot];
        }
    }
    void RHIDeviceMetal::PushFragmentUniformData(CommandBufferHandle h, uint32_t slot, const void* d, uint32_t n) {
        auto* c = GetCommand(h);
        if (c && c->renderEncoder) {
            [(id<MTLRenderCommandEncoder>) Obj(c->renderEncoder) setFragmentBytes:d length:n atIndex:slot];
        }
    }
    void RHIDeviceMetal::BindFragmentSampler(CommandBufferHandle h, uint32_t slot, TextureHandle t, SamplerHandle s) {
        auto* c = GetCommand(h);
        auto* x = GetTexture(t);
        auto* y = GetSampler(s);
        if (c && x && y && c->renderEncoder) {
            auto e = (id<MTLRenderCommandEncoder>) Obj(c->renderEncoder);
            [e setFragmentTexture:Obj(x->object) atIndex:slot];
            [e setFragmentSamplerState:Obj(y->object) atIndex:slot];
        }
    }

    void RHIDeviceMetal::BindVertexSampler(CommandBufferHandle h, uint32_t slot, TextureHandle t, SamplerHandle s) {
        auto* c = GetCommand(h);
        auto* x = GetTexture(t);
        auto* y = GetSampler(s);
        if (c && x && y && c->renderEncoder) {
            auto e = (id<MTLRenderCommandEncoder>) Obj(c->renderEncoder);
            [e setVertexTexture:Obj(x->object) atIndex:slot];
            [e setVertexSamplerState:Obj(y->object) atIndex:slot];
        }
    }

    void RHIDeviceMetal::DrawPrimitives(CommandBufferHandle h, uint32_t n, uint32_t i, uint32_t first, uint32_t fi) {
        auto* c = GetCommand(h);
        if (c && c->renderEncoder) {
            [(id<MTLRenderCommandEncoder>) Obj(c->renderEncoder) drawPrimitives:MTLPrimitiveTypeTriangle
                                                                    vertexStart:first
                                                                    vertexCount:n
                                                                  instanceCount:i
                                                                   baseInstance:fi];
        }
    }

    void RHIDeviceMetal::DrawIndexed(
        CommandBufferHandle h, uint32_t count, uint32_t instances, uint32_t first, int32_t vertexOffset, uint32_t firstInstance) {
        auto* c = GetCommand(h);
        if (c && c->renderEncoder && c->indexBuffer) {
            auto e = (id<MTLRenderCommandEncoder>) Obj(c->renderEncoder);
            [e drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                          indexCount:count
                           indexType:c->indexFormat == IndexFormat::UInt16 ? MTLIndexTypeUInt16 : MTLIndexTypeUInt32
                         indexBuffer:Obj(c->indexBuffer)
                   indexBufferOffset:c->indexOffset + first * (c->indexFormat == IndexFormat::UInt16 ? 2 : 4)
                       instanceCount:instances
                          baseVertex:vertexOffset
                        baseInstance:firstInstance];
        }
    }

    void RHIDeviceMetal::BeginComputePass(CommandBufferHandle h, const ComputePassDesc&) {
        auto* c = GetCommand(h);
        if (c) {
            c->computeEncoder = Keep([(id<MTLCommandBuffer>) Obj(c->commandBuffer) computeCommandEncoder]);
        }
    }

    void RHIDeviceMetal::EndComputePass(CommandBufferHandle h) {
        auto* c = GetCommand(h);
        if (c && c->computeEncoder) {
            [(id<MTLComputeCommandEncoder>) Obj(c->computeEncoder) endEncoding];
            Drop(c->computeEncoder);
        }
    }

    void RHIDeviceMetal::BindComputePipeline(CommandBufferHandle h, ComputePipelineHandle p) {
        auto* c = GetCommand(h);
        auto it = mComputePipelines.find(p.id);
        if (c && c->computeEncoder && it != mComputePipelines.end()) {
            [(id<MTLComputeCommandEncoder>) Obj(c->computeEncoder) setComputePipelineState:Obj(it->second.compute)];
        }
    }

    void RHIDeviceMetal::BindComputeStorageBuffer(CommandBufferHandle h, uint32_t slot, BufferHandle b) {
        auto* c = GetCommand(h);
        auto* x = GetBuffer(b);
        if (c && x && c->computeEncoder) {
            [(id<MTLComputeCommandEncoder>) Obj(c->computeEncoder) setBuffer:Obj(x->object) offset:0 atIndex:slot];
        }
    }

    void RHIDeviceMetal::BindComputeStorageTexture(CommandBufferHandle h, uint32_t slot, TextureHandle t) {
        auto* c = GetCommand(h);
        auto* x = GetTexture(t);
        if (c && x && c->computeEncoder) {
            [(id<MTLComputeCommandEncoder>) Obj(c->computeEncoder) setTexture:Obj(x->object) atIndex:slot];
        }
    }

    void RHIDeviceMetal::PushComputeUniformData(CommandBufferHandle h, uint32_t slot, const void* d, uint32_t n) {
        auto* c = GetCommand(h);
        if (c && c->computeEncoder) {
            [(id<MTLComputeCommandEncoder>) Obj(c->computeEncoder) setBytes:d length:n atIndex:slot];
        }
    }

    void RHIDeviceMetal::DispatchCompute(CommandBufferHandle h, uint32_t x, uint32_t y, uint32_t z) {
        auto* c = GetCommand(h);
        if (c && c->computeEncoder) {
            auto e = (id<MTLComputeCommandEncoder>) Obj(c->computeEncoder);
            [e dispatchThreadgroups:MTLSizeMake(x, y, z) threadsPerThreadgroup:MTLSizeMake(1, 1, 1)];
        }
    }

    void RHIDeviceMetal::WaitForIdle() {
        for (auto& [id, c] : mCommands) {
            if (c.commandBuffer) {
                [Obj(c.commandBuffer) waitUntilCompleted];
            }
        }
    }

    void RHIDeviceMetal::PushDebugGroup(CommandBufferHandle h, const char* n) {
        auto* c = GetCommand(h);
        if (c && c->renderEncoder) {
            [(id<MTLRenderCommandEncoder>) Obj(c->renderEncoder) pushDebugGroup:[NSString stringWithUTF8String:n ? n : "group"]];
        }
    }

    void RHIDeviceMetal::PopDebugGroup(CommandBufferHandle h) {
        auto* c = GetCommand(h);
        if (c && c->renderEncoder) {
            [(id<MTLRenderCommandEncoder>) Obj(c->renderEncoder) popDebugGroup];
        }
    }

    void RHIDeviceMetal::InsertDebugLabel(CommandBufferHandle h, const char* n) {
        auto* c = GetCommand(h);
        if (c && c->renderEncoder) {
            [(id<MTLRenderCommandEncoder>) Obj(c->renderEncoder) insertDebugSignpost:[NSString stringWithUTF8String:n ? n : "label"]];
        }
    }

    RHIDeviceMetal::BufferState* RHIDeviceMetal::GetBuffer(BufferHandle h) {
        auto i = mBuffers.find(h.id);
        return i == mBuffers.end() ? nullptr : &i->second;
    }
    RHIDeviceMetal::TextureState* RHIDeviceMetal::GetTexture(TextureHandle h) {
        auto i = mTextures.find(h.id);
        return i == mTextures.end() ? nullptr : &i->second;
    }

    RHIDeviceMetal::SamplerState* RHIDeviceMetal::GetSampler(SamplerHandle h) {
        auto i = mSamplers.find(h.id);
        return i == mSamplers.end() ? nullptr : &i->second;
    }

    RHIDeviceMetal::CommandState* RHIDeviceMetal::GetCommand(CommandBufferHandle h) {
        auto i = mCommands.find(h.id);
        return i == mCommands.end() ? nullptr : &i->second;
    }

    RHIDeviceMetal::PipelineState* RHIDeviceMetal::GetGraphicsPipeline(GraphicsPipelineHandle h) {
        auto i = mGraphicsPipelines.find(h.id);
        return i == mGraphicsPipelines.end() ? nullptr : &i->second;
    }

    RHIDeviceMetal::PipelineState* RHIDeviceMetal::GetComputePipeline(ComputePipelineHandle h) {
        auto i = mComputePipelines.find(h.id);
        return i == mComputePipelines.end() ? nullptr : &i->second;
    }

} // namespace golias
