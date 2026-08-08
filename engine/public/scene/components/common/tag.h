#pragma once
#include "stdafx.h"

#include "scene/components/component.h"

namespace golias {


    class Tag : public Component {
    public:
        explicit Tag(const String& tag = "Untagged");
        ~Tag() override = default;

        const String& GetTag() const;

        void SetTag(const String& tag);

    private:
        String mTag = "Untagged";
    };

} // namespace golias
