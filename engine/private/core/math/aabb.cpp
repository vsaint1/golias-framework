#include "core/math/aabb.h"

namespace golias {
    
    bool AABB::IsValid() const {
        return Min.x <= Max.x && Min.y <= Max.y && Min.z <= Max.z;
    }

    glm::vec3 AABB::GetCenter() const {
        return (Min + Max) * 0.5f;
    }

    glm::vec3 AABB::GetSize() const {
        return Max - Min;
    }

} // namespace golias
