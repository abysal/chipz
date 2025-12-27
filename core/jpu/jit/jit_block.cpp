#include "jit_block.hpp"

#include <bit>
#include <print>

namespace jip {
    uint16_t JitBlock::execute() const {
        using ptr = uint16_t (*)();
        const auto p = std::bit_cast<ptr>(m_jitted_code);
        const auto next_location = p();
        // std::println("Returned from JIT block, jumping to: 0x{:x}", next_location);
        return next_location;
    }
} // namespace jip