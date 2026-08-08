#pragma once
#include "stdafx.h"

namespace golias {

    enum class ShaderStage { Vertex, Fragment, Geometry, Compute };

    enum class ShaderSourceType { GLSL, HLSL, SPIRV };

    struct ShaderDesc {
        std::string Path;

        ShaderStage Stage = ShaderStage::Vertex;

        std::string EntryPoint = "main";

        ShaderSourceType Source = ShaderSourceType::SPIRV;
    };

    class Shader {
    public:
        virtual ~Shader() = default;

        virtual ShaderStage GetStage() const = 0;
    };

} // namespace golias
