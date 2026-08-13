#pragma once
#include "stdafx.h"
#include <limits>

#include <glm/glm.hpp>

namespace golias {

    // Axis-aligned bounding box.
    struct AABB {
        glm::vec3 Min = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 Max = glm::vec3(std::numeric_limits<float>::lowest());

        bool IsValid() const;

        glm::vec3 GetCenter() const;

        glm::vec3 GetSize() const;

        template <typename TVertexContainer>
        static AABB FromVertices(const TVertexContainer& vertices) {
            AABB bounds;
            for (const auto& vertex : vertices) {
                bounds.Min = glm::min(bounds.Min, vertex.Position);
                bounds.Max = glm::max(bounds.Max, vertex.Position);
            }
            
            return bounds;
        }

    };

} // namespace golias
