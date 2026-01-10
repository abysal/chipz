#include "exec_manager.hpp"

#include "comp/emitter.hpp"
#include "comp/linear_scan_allocator.hpp"
#include "disassembler/disassembler.hpp"
#include "ir/hlir.hpp"

#include <print>

namespace cip {
    void ExecManager::execute_from(const u16 program_counter, const MemoryStream& prog_mem) {
        if (this->m_compiled_blocks.contains(program_counter)) {
            this->m_compiled_blocks[program_counter].execute();
            return;
        }

        chip::Disassembler disassembler{ prog_mem, program_counter };
        const auto disassembly = disassembler.create_block_disassembly();

        std::println("{}", disassembly.as_string());

        hlir::HLIR hlir_code{};
        hlir_code.translate(disassembly);

        comp::LinearScanAllocator linear_scan_allocator{};
        linear_scan_allocator.trace_accesses(hlir_code);

        comp::Emitter emitter{ std::move(linear_scan_allocator) };
    }
} // namespace cip