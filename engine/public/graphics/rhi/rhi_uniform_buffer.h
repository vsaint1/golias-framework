#pragma once
#include "stdafx.h"
#include "graphics/rhi/rhi_shader.h"

namespace golias {

    // Describes a single uniform buffer (UBO) exposed to a shader stage.
    struct UniformBufferDesc {
        std::string Name;

        uint32_t Binding = 0;

        size_t Size = 0;

        ShaderStage Stage = ShaderStage::Vertex;
    };

    // A single UBO. Backend implementations own the per-frame GPU buffers.
    class UniformBuffer {
    public:
        virtual ~UniformBuffer() = default;

        virtual const std::string& GetName() const = 0;

        virtual uint32_t GetBinding() const = 0;

        virtual size_t GetSize() const = 0;

        virtual ShaderStage GetStage() const = 0;
    };

    // A set of UBOs grouped together.
    class UniformBufferSet {
    public:
        virtual ~UniformBufferSet() = default;

        virtual Ref<UniformBuffer> Get(const std::string& name) const = 0;

        virtual void SetData(const std::string& name, uint32_t frameIndex, const void* data, size_t size, size_t offset = 0) = 0;
    };

} // namespace golias
