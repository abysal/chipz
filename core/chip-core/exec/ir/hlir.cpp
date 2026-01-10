#include "hlir.hpp"

#include "constants.hpp"

#include <cassert>
#include <print>

namespace cip::hlir {
    HLIR::HLIR() { this->m_blocks.resize(1); }

    void HLIR::translate(const chip::Disassembly& disassembly) {
        for (const auto& instr : disassembly) {
            switch (instr.type()) {
            case chip::InstructionType::LoadImm:
            case chip::InstructionType::LoadImmI:
                this->emit_load(instr);
                continue;
            case chip::InstructionType::Draw:
                this->emit_dxyn(instr);
                continue;
            case chip::InstructionType::Jump:
                this->emit_jmp(instr);
                continue;
            default:
                std::println("Unhandled instruction: {}", static_cast<uint16_t>(instr.type()));
            }
        }
    }

    void HLIR::emit_load(const chip::Instruction instruction) {
        if (instruction.type() == chip::InstructionType::LoadImm) {
            const auto vx_reg = this->m_reg_allocator.cpu_reg(instruction.vx_reg());
            this->emit(Mov{ vx_reg, instruction.imm() });
        } else if (instruction.type() == chip::InstructionType::LoadImmI) {
            const auto index_reg = this->m_reg_allocator.cpu_special_reg(SpecialCPURegs::IndexPointer);
            this->emit(Mov{ index_reg, instruction.imm() });
        } else [[unlikely]] {
            throw std::logic_error("This somehow happened");
        }
    }

    void HLIR::emit_dxyn(const chip::Instruction instruction) {
        const auto x_reg = this->m_reg_allocator.cpu_reg(instruction.vx_reg());
        const auto y_reg = this->m_reg_allocator.cpu_reg(instruction.vy_reg());
        const auto size = instruction.imm_as<u8>();
        const auto index_reg = this->m_reg_allocator.cpu_special_reg(SpecialCPURegs::IndexPointer);

        const auto extended_index = this->m_reg_allocator.new_64();
        const auto memory_reg = this->m_reg_allocator.new_ptr();
        const auto display_reg = this->m_reg_allocator.new_ptr();
        const auto wrapped_x = this->m_reg_allocator.new_32();
        const auto wrapped_y = this->m_reg_allocator.new_32();
        const auto row_tracker = this->m_reg_allocator.new_32();
        const auto col_tracker = this->m_reg_allocator.new_32();
        const auto current_byte = this->m_reg_allocator.new_8();
        const auto large_current_byte = this->m_reg_allocator.new_32();
        const auto masked_byte = this->m_reg_allocator.new_32();
        const auto mask_store = this->m_reg_allocator.new_32();
        const auto y_index_reg = this->m_reg_allocator.new_32();
        const auto x_index_reg = this->m_reg_allocator.new_32();

        const auto final_display_index = this->m_reg_allocator.new_32();

        this->emit(
            UpCastReg{ wrapped_x, x_reg },
            UpCastReg{ wrapped_y, y_reg },
            Mov{ row_tracker, 0 },
            GetCPUMemory{ memory_reg },
            GetDisplayMemory{ display_reg },
            UpCastReg{ extended_index, index_reg },
            Add{ memory_reg, extended_index },
            And{ wrapped_x, width - 1 },
            And{ wrapped_y, height - 1 }
        );

        const auto row_start_block = this->new_block();

        const auto col_block = this->new_block();
        const auto col_block_end = this->new_block();
        const auto row_end_block = this->new_block();

        const auto exit_block = this->new_block();

        this->use_block(row_start_block);

        {
            this->emit(
                Mov{ col_tracker, 0 },
                ReadPtr{ current_byte, memory_reg },
                UpCastReg{ large_current_byte, current_byte },
                Add{ memory_reg, 1 },
                Mov{ y_index_reg, wrapped_y },
                Add{ y_index_reg, row_tracker },
                JmpGEBlock{ y_index_reg,
                            height,
                            exit_block.block_index }, // If the y index we wanted to write to is higher than
                                                      // the display can count, we just exit draw the loop
                Mul{ y_index_reg, width } // this mul saves us one instruction in the inner loop of the fn lmao
            );

            {
                this->use_block(col_block);

                this->emit(
                    Mov{ masked_byte, large_current_byte },
                    Mov{ mask_store, 0x80 },
                    Shr{ mask_store, col_tracker },
                    And{ masked_byte, mask_store },
                    JmpZBlock{ masked_byte, col_block_end.block_index },

                    Mov{ x_index_reg, wrapped_x },
                    Add{ x_index_reg, col_tracker },
                    JmpGEBlock{ x_index_reg, width, row_end_block.block_index },

                    Mov{ final_display_index, x_index_reg },
                    Add{ final_display_index, y_index_reg },
                    ReadPtr{ current_byte, display_reg, final_display_index },
                    Xor{ current_byte, 1 },
                    WritePtr{ current_byte, display_reg, final_display_index }
                );

                this->use_block(col_block_end);
                this->emit(
                    Add{ col_tracker, 1 }, //
                    JmpLBlock{ col_tracker, 8, col_block.block_index }
                );
            }

            this->use_block(row_end_block);

            this->emit(Add{ row_tracker, 1 }, JmpLBlock{ row_tracker, size, row_start_block.block_index });
        }

        this->use_block(exit_block);
    }

    void HLIR::emit_jmp(const chip::Instruction instruction) {
        const auto instruction_pointer = this->m_reg_allocator.cpu_special_reg(SpecialCPURegs::ProgramCounter);

        this->emit(
            Mov{ instruction_pointer, instruction.imm() }, //
            ReturnToJit{}
        );
    }

    HLIR::BlockHandle HLIR::new_block() {
        const BlockHandle handle{ .block_index = ++this->m_active_block };
        this->m_blocks.emplace_back();
        return handle;
    }

    void HLIR::use_block(const BlockHandle block) { this->m_active_block = block.block_index; }

} // namespace cip::hlir
