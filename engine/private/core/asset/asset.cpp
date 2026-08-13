#include "core/asset/asset.h"

namespace golias {

    const String& Asset::GetPath() const {
        return mPath;
    }

    const UUID& Asset::GetUUID() const {
        return mUUID;
    }

    void Asset::SetUUID(const UUID& uuid) {
        mUUID = uuid;
    }

    void Asset::SetPath(const String& path) {
        mPath = path;
    }

} // namespace golias
