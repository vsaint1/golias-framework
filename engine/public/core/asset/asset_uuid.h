#pragma once

#include "stdafx.h"

namespace golias {

    using UUID = String;

    UUID Generate_UUID();
    
    bool IsValid_UUID(const UUID& uuid);

} // namespace golias
