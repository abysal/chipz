#pragma once
#include "constants.hpp"
#include "util/display.hpp"
#include "util/register.hpp"
#include "util/types.hpp"

#include <array>

namespace cip {

    using GpRegister = Register<u8>;
    using IndexRegister = Register<u16>;
    using PcRegister = Register<u16>;

    // core state cannot move since it is referenced statically as a pointer inside the emitted assembly blocks
    class CoreState {
    public:
        CoreState() = default;
        CoreState(const CoreState&) = delete;
        CoreState(CoreState&&) = delete;
        CoreState& operator=(const CoreState&) = delete;
        CoreState& operator=(CoreState&&) = delete;

    public:
        std::array<GpRegister, gp_count> gp_registers{};
        IndexRegister index_register{};
        PcRegister program_counter{};
        std::array<u8, memory_size> memory{};
        Display chip_display{};
    };
} // namespace cip