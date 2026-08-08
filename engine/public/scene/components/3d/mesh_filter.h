#pragma once
#include "stdafx.h"

#include "scene/components/component.h"
#include "scene/mesh_library.h"

namespace golias {


    
    class MeshFilter : public Component {
    public:
        MeshFilter() = default;
        ~MeshFilter() override = default;

        // Shares an existing mesh asset.
        void SetMesh(const Ref<MeshData>& mesh);

        // Copies `mesh` into a new shared asset.
        void SetMesh(const MeshData& mesh);

        // Builds a shared asset from a built-in primitive.
        void SetMesh(PrimitiveType type);

        const Ref<MeshData>& GetMesh() const;

    private:
        Ref<MeshData> mMesh = nullptr;
    };

} // namespace golias
