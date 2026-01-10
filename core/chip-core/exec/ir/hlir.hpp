#pragma once
#include "chip-core/exec/disassembler/disassembly.hpp"
#include "hlir/instruction.hpp"
#include "register_allocator.hpp"

#include <memory>
#include <vector>

namespace cip::comp {
    class LinearScanAllocator;
}

namespace cip::hlir {

    // TODO: Apply optimizations to the IR later down the line.
    class HLIR {
    public:
        HLIR();
        void translate(const chip::Disassembly& disassembly);

        [[nodiscard]] const auto& allocator() const noexcept { return this->m_reg_allocator; }

        [[nodiscard]] const auto& blocks() const noexcept { return this->m_blocks; }

    private:
        struct BlockHandle {
            u32 block_index{};
        };

        void emit_load(chip::Instruction instruction);
        void emit_dxyn(chip::Instruction instruction);
        void emit_jmp(chip::Instruction instruction);

        template <typename... Args>
            requires((std::is_base_of_v<Instruction, Args> && ...))
        void emit(Args&&... args) {
            auto& block = this->m_blocks[m_active_block];
            (block.instructions.emplace_back(std::make_unique<std::decay_t<Args>>(std::forward<Args>(args))), ...);
        }

        BlockHandle new_block();

        void use_block(BlockHandle block);

    private:
        struct IRBlock {
            std::vector<std::unique_ptr<Instruction>> instructions;
        };

        u32 m_active_block{ 0 };
        u32 m_next_block{ 0 };
        std::vector<IRBlock> m_blocks{};
        RegisterAllocator m_reg_allocator{};
    };

} // namespace cip::hlir
