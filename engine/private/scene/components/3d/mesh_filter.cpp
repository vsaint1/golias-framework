#include "scene/components/3d/mesh_filter.h"

namespace golias {

    void MeshFilter::SetMesh(const Ref<Mesh>& mesh) {
        mMesh = mesh;
    }

    const Ref<Mesh>& MeshFilter::GetMesh() const {
        return mMesh;
    }

} // namespace golias
