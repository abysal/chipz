#include "chip_core.hpp"

#include <algorithm>

namespace cip {
    void ChipCore::load_rom(const std::span<const u8> memory, const u16 start_pc) {
        std::ranges::copy(memory, this->memory.begin() + start_pc);
        this->program_counter = start_pc;
    }

    void ChipCore::execute() {
        while (this->m_running.load(std::memory_order_relaxed)) {
            this->m_exec_manager.execute_from(
                this->program_counter, MemoryStream{ std::span{ this->memory }.subspan(this->program_counter.value()) }
            );
        }
    }

    void ChipCore::set_running(const bool value) { this->m_running = value; }
} // namespace cip