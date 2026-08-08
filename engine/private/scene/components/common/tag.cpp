#include "scene/components/common/tag.h"

namespace golias {

    Tag::Tag(const String& tag) : mTag(tag) {
    }

    const String& Tag::GetTag() const {
        return mTag;
    }

    void Tag::SetTag(const String& tag) {
        mTag = tag;
    }

} // namespace golias
