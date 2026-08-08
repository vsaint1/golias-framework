#pragma once
#include "stdafx.h"
#include "graphics/rhi/rhi_shader.h"
#include "graphics/rhi/rhi_uniform_buffer.h"

namespace golias {

    enum class PrimitiveTopology { TriangleList, TriangleStrip, LineList, LineStrip, PointList };

    enum class CullMode { None, Front, Back };

    enum class FrontFace { CounterClockwise, Clockwise };

    enum class PolygonMode { Fill, Line, Point };

    enum class VertexInputRate { PerVertex, PerInstance };

    enum class VertexFormat { Float, Float2, Float3, Float4 };

    // A single vertex attribute inside a vertex buffer layout.
    struct VertexAttribute {
        uint32_t Location = 0;

        VertexFormat Format = VertexFormat::Float3;

        size_t Offset = 0;
    };

    // Describes one vertex buffer binding (e.g. per-vertex data).
    struct VertexBufferLayout {
        uint32_t Stride = 0;

        VertexInputRate InputRate = VertexInputRate::PerVertex;

        std::vector<VertexAttribute> Attributes = {};
    };

    // The state used to build a graphics pipeline.
    struct GraphicsPipelineStateDesc {
        std::vector<Ref<Shader>> Shaders = {};

        Ref<UniformBufferSet> UniformBuffers = nullptr;

        std::vector<VertexBufferLayout> VertexLayouts = {};

        PrimitiveTopology Topology = PrimitiveTopology::TriangleList;

        CullMode Cull = CullMode::Back;

        FrontFace Winding = FrontFace::Clockwise;

        PolygonMode FillMode = PolygonMode::Fill;

        bool DepthTest = false;

        bool DepthWrite = false;
    };

    class GraphicsPipeline {
    public:
        virtual ~GraphicsPipeline() = default;
    };

} // namespace golias
