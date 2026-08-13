#include "core/asset/asset_uuid.h"

#include <random>

namespace golias {

    UUID Generate_UUID() {
        std::random_device random;
        std::mt19937_64 generator(random());
        std::uniform_int_distribution<uint32_t> distribution;

        uint32_t parts[4] = {distribution(generator), distribution(generator), distribution(generator), distribution(generator)};
        parts[1]          = (parts[1] & 0xFFFF0FFFu) | 0x00004000u;
        parts[2]          = (parts[2] & 0x3FFFFFFFu) | 0x80000000u;

        return fmt::format("{:08x}-{:04x}-{:04x}-{:04x}-{:08x}{:04x}",
                           parts[0],
                           parts[1] >> 16,
                           parts[1] & 0xFFFFu,
                           parts[2] >> 16,
                           parts[2] & 0xFFFFu,
                           parts[3] & 0xFFFFu);
    }

    bool IsValid_UUID(const UUID& uuid) {
        return uuid.size() == 36 && uuid[8] == '-' && uuid[13] == '-' && uuid[18] == '-' && uuid[23] == '-';
    }

} // namespace golias
