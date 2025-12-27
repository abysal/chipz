#pragma once
#include "cpu/display.hpp"
#include "register.hpp"
#include "util/static_stack.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace jip {
    constexpr static uint8_t GPRegCount = 18;
    using StackType = cip::StaticVector<uint16_t, 16, uint8_t>;

    struct CoreState {
        CoreState() = default;
        ~CoreState() = default;
        CoreState(const CoreState&) = delete;
        CoreState& operator=(const CoreState&) = delete;
        CoreState(CoreState&&) = delete;
        CoreState& operator=(CoreState&&) = delete;

        std::array<Register<uint8_t>, GPRegCount> registers{};
        Register<uint16_t> index_register{};
        Register<uint16_t> instruction_pointer{};
        StackType stack{};
        std::array<uint8_t, 0x2000> memory{};
        alignas(64) cip::Display core_display{};
    };

} // namespace jip