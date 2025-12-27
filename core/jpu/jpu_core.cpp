#include "jpu_core.hpp"

namespace jip {

    void JpuCore::load(std::span<const uint8_t> memory) noexcept {
        this->instruction_pointer.set(0x200);

        std::ranges::copy(memory, this->memory.begin() + this->instruction_pointer.value());
        this->m_jit.set_state(this);
        const auto block = this->m_jit.compile_block(
            this->instruction_pointer.value(),
            MemoryStream{ std::span{ this->memory }.subspan(this->instruction_pointer.value()) }
        );
        this->m_jit.execute_loop(0x200, block);
    }
} // namespace jip