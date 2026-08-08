#include "scene/components/3d/mesh_filter.h"

namespace golias {

    void MeshFilter::SetMesh(const Ref<MeshData>& mesh) {
        mMesh = mesh;
    }

    void MeshFilter::SetMesh(const MeshData& mesh) {
        mMesh = std::make_shared<MeshData>(mesh);
    }

    void MeshFilter::SetMesh(PrimitiveType type) {
        SetMesh(MeshLibrary::Create(type));
    }

    const Ref<MeshData>& MeshFilter::GetMesh() const {
        return mMesh;
    }

} // namespace golias
