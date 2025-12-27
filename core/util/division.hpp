#pragma once
#include <cstdint>

namespace jip {
    struct Div8Info {
        uint16_t mul;
        uint8_t shift;
    };

    Div8Info compute_magic_for_d(uint8_t divisor) noexcept;

} // namespace jip