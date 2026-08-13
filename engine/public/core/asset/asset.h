#pragma once

#include "stdafx.h"
#include "core/asset/asset_uuid.h"

namespace golias {

    class Asset {
    public:
        virtual ~Asset() = default;

        const String& GetPath() const;

        void SetPath(const String& path);
        
        const UUID& GetUUID() const;

        void SetUUID(const UUID& uuid);


    private:
        String mPath;
        UUID mUUID;
    };

} // namespace golias
