#include "scene/components/component.h"

#include "scene/game_object.h"

namespace golias {

    GameObject* Component::GetOwner() const {
        return mOwner;
    }

    Scene* Component::GetScene() const {
        return mOwner != nullptr ? mOwner->GetScene() : nullptr;
    }

    void Component::SetOwner(GameObject* owner) {
        mOwner = owner;
    }

} // namespace golias
