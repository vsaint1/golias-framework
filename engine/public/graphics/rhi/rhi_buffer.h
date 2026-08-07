#pragma once
#include "stdafx.h"

namespace golias {
    enum class BufferType { Vertex, Index, Uniform, Storage };

    enum class BufferUsage { Static, Dynamic, Stream };

    struct BufferDesc {
        BufferType type;
        BufferUsage usage;
        size_t size;
        const void* data = nullptr;
    };

    class Buffer {
    public:
        virtual ~Buffer() = default;

        virtual void SetData(const void* data, size_t size, size_t offset = 0) = 0;

        virtual size_t GetSize() const  = 0;

        virtual BufferType GetType() const = 0;

        static Ref<Buffer> Create(const BufferDesc& desc);
    };
} // namespace golias
