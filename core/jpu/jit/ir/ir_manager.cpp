#include "ir_manager.hpp"

#include <algorithm>
#include <print>
#include <stdexcept>
#include <utility>

namespace jip {
    uint32_t IRManager::alloc_temp_for_reg(IRReg reg) noexcept {
        for (const auto& [index, reg_value] : this->m_register_temps) {
            if (reg_value == reg) {
                return index;
            }
        }

        const auto index = this->new_temp();
        this->m_register_temps.emplace_back(index, reg);
        return index;
    }

    void IRManager::emit(const Instruction instr, const uint16_t current_ip) {
        if (std::ranges::contains(this->m_new_block_points, current_ip)) {
            const auto block = this->new_block();
            block.use_block();
            this->m_block_point_to_block_index[current_ip] = block.index();
        }

        if (this->m_block_switch_counter) {
            const auto counter = --this->m_block_switch_counter;

            if (counter == 0) {
                this->m_handle_to_switch.use_block();
            }
        }

        if (instr.is_skip_next()) {
            assert(std::ranges::contains(this->m_new_block_points, current_ip + 4));
            const auto block = this->new_block();
            this->m_block_point_to_block_index[current_ip + 4] = block.index();
            this->m_block_switch_counter = 2;
            this->m_handle_to_switch = block;
        }

        switch (instr.type()) {
        case InstructionType::Native:
            if (instr.immediate() == 0xE0) {
                this->emit_instruction({ IROpcode::ClearDisplayMemory });
                return;
            }
            if (instr.immediate() == 0xEE) {
                this->emit_return(instr);
                return;
            }
            throw std::logic_error(std::format("Unhandled native call, {:x}", instr.immediate()));
        case InstructionType::LoadImm:
            this->emit_load_imm(instr);
            return;
        case InstructionType::LoadImmI:
            this->emit_index_imm(instr);
            return;
        case InstructionType::AddImm:
            this->emit_add_imm(instr);
            return;
        case InstructionType::Draw:
            this->emit_dxyn(instr);
            return;
        case InstructionType::Jump:
            this->emit_jump(instr);
            return;
        case InstructionType::SkipEqRegImm:
            this->emit_skip_reg_eq_imm(instr);
            return;
        case InstructionType::SkipNeRegImm:
            this->emit_skip_reg_ne_imm(instr);
            return;
        case InstructionType::SkipEqRegReg:
            this->emit_skip_reg_eq_reg(instr);
            return;
        case InstructionType::SkipNeRegReg:
            this->emit_skip_reg_ne_reg(instr);
            return;
        case InstructionType::Call:
            this->emit_call(instr, current_ip);
            return;
        case InstructionType::MovReg:
            this->emit_mov_reg(instr);
            return;
        case InstructionType::RegOr:
            this->emit_reg_or(instr);
            return;
        case InstructionType::RegAnd:
            this->emit_reg_and(instr);
            return;
        case InstructionType::RegXor:
            this->emit_reg_xor(instr);
            return;
        case InstructionType::RegAddYX:
            this->emit_reg_add_xy(instr);
            return;
        case InstructionType::RegSubXY:
            this->emit_reg_sub_xy(instr);
            return;
        case InstructionType::RegSubYX:
            this->emit_reg_sub_yx(instr);
            return;
        case InstructionType::RegShrXY:
            this->emit_reg_shr_xy(instr);
            return;
        case InstructionType::RegShlXY:
            this->emit_reg_shl_xy(instr);
            return;
        case InstructionType::RangeRead:
            this->emit_range_read(instr);
            return;
        case InstructionType::RangeWrite:
            // this->emit_range_write(instr);
            // return;
        case InstructionType::BCD:
            // this->emit_bcd(instr);
            // return;
        default:
            std::println("Unhandled instruction type: {:x}", static_cast<uint32_t>(instr.type()));
            // throw std::runtime_error("Unhandled instruction type");
        }
    }

    void IRManager::emit_load_imm(const Instruction instr) {
        assert(instr.type() == InstructionType::LoadImm);
        const auto reg = static_cast<IRReg>(instr.used_regs()[0]);
        const auto index = this->alloc_temp_for_reg(reg);

        this->emit_instruction(
            IRInstruction{
                .code = IROpcode::LoadImmediate,
                .vx = RegisterPointer{ false, static_cast<uint32_t>(index) },
                .immediate = instr.immediate()
        }
        );
    }

    void IRManager::emit_index_imm(const Instruction instr) {
        assert(instr.type() == InstructionType::LoadImmI);
        const auto immediate = instr.immediate();
        const auto index = this->alloc_temp_for_reg(IRReg::IN);
        this->emit_instruction(
            IRInstruction{
                .code = IROpcode::LoadImmediate,
                .vx = RegisterPointer{ false, static_cast<uint32_t>(index) },
                .immediate = immediate
        }
        );
    }

    void IRManager::emit_add_imm(const Instruction instr) {
        assert(instr.type() == InstructionType::AddImm);
        const auto reg = static_cast<IRReg>(instr.used_regs()[0]);
        const auto index = this->alloc_temp_for_reg(reg);

        this->emit_instruction(
            IRInstruction{
                .code = IROpcode::AddImm,
                .vx = RegisterPointer{ false, static_cast<uint32_t>(index) },
                .vy = RegisterPointer{ false, static_cast<uint32_t>(index) },
                .immediate = instr.immediate()
        }
        );

        // The immediate add instruction doesn't set the overflow flag
    }

    void IRManager::emit_dxyn(const Instruction instr) {
        assert(instr.type() == InstructionType::Draw);

        const auto x_reg = static_cast<IRReg>(instr.used_regs()[0]);
        const auto y_reg = static_cast<IRReg>(instr.used_regs()[1]);
        const auto height = instr.immediate();

        const auto sprite_byte_reg = this->new_temp();
        const auto dy_reg = this->new_temp();
        const auto dx_reg = this->new_temp();
        const auto byte_scratch_reg = this->new_temp();

        const auto index_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(IRReg::IN) };
        const auto x_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(x_reg) };
        const auto y_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(y_reg) };
        const auto sprite_byte_pointer = RegisterPointer{ true, static_cast<uint32_t>(sprite_byte_reg) };
        const auto dy_pointer = RegisterPointer{ true, static_cast<uint32_t>(dy_reg) };
        const auto dx_pointer = RegisterPointer{ true, static_cast<uint32_t>(dx_reg) };
        const auto byte_scratch_pointer = RegisterPointer{ true, static_cast<uint32_t>(byte_scratch_reg) };

        this->emit_instruction({ IROpcode::AndImm, x_pointer, x_pointer, 63 });
        this->emit_instruction({ IROpcode::AndImm, y_pointer, y_pointer, 31 });

        for (uint16_t y = 0; y < height; ++y) {
            this->emit_instruction({ IROpcode::LoadByteFromI, index_pointer, sprite_byte_pointer, y });

            this->emit_instruction({ IROpcode::AddImm, y_pointer, dy_pointer, y });
            this->emit_instruction({ IROpcode::AndImm, dy_pointer, dy_pointer, 31 });

            for (uint16_t x = 0; x < 8; ++x) {
                this->emit_instruction({ IROpcode::AddImm, x_pointer, dx_pointer, x });

                this->emit_instruction(
                    { IROpcode::AndImm, sprite_byte_pointer, byte_scratch_pointer, static_cast<uint16_t>(0x80 >> x) }
                );

                const auto succeed_block = this->new_block();
                const auto fail_block = this->new_block();

                this->emit_instruction(
                    { .code = IROpcode::JmpZ, .vx = byte_scratch_pointer, .immediate = fail_block.m_index }
                );
                // code for actually drawing

                succeed_block.use_block();

                this->emit_instruction(
                    { IROpcode::AndImm, dx_pointer, byte_scratch_pointer, static_cast<uint16_t>(~63) }
                );
                this->emit_instruction(
                    { .code = IROpcode::JmpNZ, .vx = byte_scratch_pointer, .immediate = fail_block.m_index }
                );

                this->emit_instruction(
                    { IROpcode::AndImm, dy_pointer, byte_scratch_pointer, static_cast<uint16_t>(~31) }
                );
                this->emit_instruction(
                    { .code = IROpcode::JmpNZ, .vx = byte_scratch_pointer, .immediate = fail_block.m_index }
                );

                this->emit_instruction(
                    { IROpcode::XorDisplayMemory, dx_pointer, dy_pointer, static_cast<uint16_t>(fail_block.index()) }
                );

                fail_block.use_block();
            }
        }
    }

    void IRManager::emit_dxyn_8_bit(const Instruction instr) {
        assert(instr.type() == InstructionType::Draw);

        const auto x_reg = static_cast<IRReg>(instr.used_regs()[0]);
        const auto y_reg = static_cast<IRReg>(instr.used_regs()[1]);
        const auto height = instr.immediate();

        const auto splat_byte_reg = this->new_temp();
        const auto sprite_byte_reg = this->new_temp();

        const auto index_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(IRReg::IN) };
        const auto x_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(x_reg) };
        const auto y_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(y_reg) };
        const auto sprite_byte_pointer = RegisterPointer{ true, static_cast<uint32_t>(sprite_byte_reg) };
        const auto splat_byte_pointer = RegisterPointer{ true, static_cast<uint32_t>(splat_byte_reg) };

        this->emit_instruction({ IROpcode::AndImm, x_pointer, x_pointer, 63 });
        this->emit_instruction({ IROpcode::AndImm, y_pointer, y_pointer, 31 });

        for (uint16_t y = 0; y < height; ++y) {
            this->emit_instruction({ IROpcode::LoadByteFromI, index_pointer, sprite_byte_pointer, y });

            // this->emit_instruction({ IROpcode::ExtendReg64, sprite_byte_pointer, sprite_byte_pointer });
            // this->emit_instruction({ IROpcode::MulImm64, sprite_byte_pointer, sprite_byte_pointer,
            //                          0x0101010101010101 }); // this splats 8 bits into 8 bytes
        }
    }

    void IRManager::emit_jump(const Instruction instr) {
        assert(instr.type() == InstructionType::Jump);
        this->emit_jit_jump(instr);
    }

    void IRManager::emit_self_jump(const Instruction instr) {
        const auto target_block = this->m_block_point_to_block_index.at(instr.immediate());
        this->emit_instruction({ .code = IROpcode::JmpBlock, .immediate = target_block });
    }

    void IRManager::emit_jit_jump(const Instruction instr) {
        const auto target_ip = instr.immediate();
        this->emit_instruction({ .code = IROpcode::JmpJit, .immediate = target_ip });
    }

    void IRManager::emit_skip_reg_eq_imm(const Instruction instr) {
        assert(instr.type() == InstructionType::SkipEqRegImm);
        assert(this->m_block_switch_counter == 2);
        const auto x_reg = static_cast<IRReg>(instr.used_regs()[0]);

        const auto x_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(x_reg) };
        this->emit_instruction(
            { .code = IROpcode::JmpEqImm,
              .vx = x_pointer,
              .immediate = instr.immediate(),
              .immediate_2 = this->m_handle_to_switch.index() }
        );
    }

    void IRManager::emit_skip_reg_ne_imm(const Instruction instr) {
        assert(instr.type() == InstructionType::SkipNeRegImm);
        assert(this->m_block_switch_counter == 2);
        const auto x_reg = static_cast<IRReg>(instr.used_regs()[0]);

        const auto x_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(x_reg) };
        this->emit_instruction(
            { .code = IROpcode::JmpNeImm,
              .vx = x_pointer,
              .immediate = instr.immediate(),
              .immediate_2 = this->m_handle_to_switch.index() }
        );
    }

    void IRManager::emit_skip_reg_eq_reg(const Instruction instr) {
        assert(instr.type() == InstructionType::SkipEqRegReg);
        assert(this->m_block_switch_counter == 2);
        const auto x_reg = static_cast<IRReg>(instr.used_regs()[0]);
        const auto y_reg = static_cast<IRReg>(instr.used_regs()[1]);

        const auto x_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(x_reg) };
        const auto y_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(y_reg) };
        this->emit_instruction(
            { .code = IROpcode::JmpEqReg,
              .vx = x_pointer,
              .vy = y_pointer,
              .immediate = this->m_handle_to_switch.index() }
        );
    }

    void IRManager::emit_skip_reg_ne_reg(const Instruction instr) {
        assert(instr.type() == InstructionType::SkipNeRegReg);
        assert(this->m_block_switch_counter == 2);
        const auto x_reg = static_cast<IRReg>(instr.used_regs()[0]);
        const auto y_reg = static_cast<IRReg>(instr.used_regs()[1]);

        const auto x_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(x_reg) };
        const auto y_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(y_reg) };
        this->emit_instruction(
            { .code = IROpcode::JmpNeReg,
              .vx = x_pointer,
              .vy = y_pointer,
              .immediate = this->m_handle_to_switch.index() }
        );
    }

    void IRManager::emit_call(const Instruction instr, const uint16_t current_ip) {
        assert(instr.type() == InstructionType::Call);

        const auto stack_offset_reg = this->new_temp();

        const auto stack_offset_pointer = RegisterPointer{ true, stack_offset_reg };
        const auto target_address = instr.immediate();
        const auto return_address = static_cast<uint16_t>(current_ip + 2);
        this->emit_instruction({ IROpcode::ReadStackOffset, stack_offset_pointer });
        this->emit_instruction({ IROpcode::AddImm, stack_offset_pointer, stack_offset_pointer, 1 });
        this->emit_instruction({ IROpcode::WriteStackOffset, stack_offset_pointer });
        this->emit_instruction(
            { .code = IROpcode::WriteToStackWithOffset, .vx = stack_offset_pointer, .immediate = return_address }
        );
        this->emit_instruction({ .code = IROpcode::JmpJit, .immediate = target_address });
    }

    void IRManager::emit_return(const Instruction instr) {
        assert(instr.type() == InstructionType::Native);
        assert(instr.immediate() == 0xEE);
        const auto stack_offset_reg = this->new_temp();
        const auto jump_scratch_reg = this->new_temp();
        const auto stack_offset_pointer = RegisterPointer{ true, stack_offset_reg };
        const auto jump_scratch_pointer = RegisterPointer{ true, jump_scratch_reg };

        this->emit_instruction({ IROpcode::ReadStackOffset, stack_offset_pointer });
        this->emit_instruction(
            { IROpcode::JumpToStackWithOffsetAndDecrement, stack_offset_pointer, jump_scratch_pointer }
        );
    }

    void IRManager::emit_mov_reg(const Instruction instr) {
        assert(instr.type() == InstructionType::MovReg);

        const auto dst = static_cast<IRReg>(instr.used_regs()[0]);
        const auto src = static_cast<IRReg>(instr.used_regs()[1]);

        const auto dst_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(dst) };
        const auto src_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(src) };

        this->emit_instruction({ IROpcode::LoadReg, dst_pointer, src_pointer });
    }

    void IRManager::emit_reg_or(const Instruction instr) {
        assert(instr.type() == InstructionType::RegOr);

        const auto dst = static_cast<IRReg>(instr.used_regs()[0]);
        const auto src = static_cast<IRReg>(instr.used_regs()[1]);
        const auto dst_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(dst) };
        const auto src_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(src) };

        this->emit_instruction({ IROpcode::OrRegReg, dst_pointer, src_pointer });
    }

    void IRManager::emit_reg_and(const Instruction instr) {
        assert(instr.type() == InstructionType::RegAnd);
        const auto dst = static_cast<IRReg>(instr.used_regs()[0]);
        const auto src = static_cast<IRReg>(instr.used_regs()[1]);

        const auto dst_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(dst) };
        const auto src_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(src) };

        this->emit_instruction({ IROpcode::AndRegReg, dst_pointer, src_pointer });
    }

    void IRManager::emit_reg_xor(const Instruction instr) {
        assert(instr.type() == InstructionType::RegXor);
        const auto dst = static_cast<IRReg>(instr.used_regs()[0]);
        const auto src = static_cast<IRReg>(instr.used_regs()[1]);

        const auto dst_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(dst) };
        const auto src_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(src) };

        this->emit_instruction({ IROpcode::XorRegReg, dst_pointer, src_pointer });
    }

    void IRManager::emit_reg_add_xy(const Instruction instr) {
        assert(instr.type() == InstructionType::RegAddYX);
        const auto dst = static_cast<IRReg>(instr.used_regs()[0]);
        const auto src = static_cast<IRReg>(instr.used_regs()[1]);

        const auto dst_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(dst) };
        const auto src_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(src) };
        const auto vF = RegisterPointer{ false, this->alloc_temp_for_reg(IRReg::VF) };

        this->emit_instruction({ IROpcode::Add, dst_pointer, src_pointer });
        this->emit_instruction({ .code = IROpcode::FlagRegisterCheck, .vx = vF, .immediate = 0xADD });
    }

    void IRManager::emit_reg_sub_xy(const Instruction instr) {
        assert(instr.type() == InstructionType::RegSubXY);
        const auto dst = static_cast<IRReg>(instr.used_regs()[0]);
        const auto src = static_cast<IRReg>(instr.used_regs()[1]);

        const auto dst_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(dst) };
        const auto src_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(src) };
        const auto vF = RegisterPointer{ false, this->alloc_temp_for_reg(IRReg::VF) };

        this->emit_instruction(
            {
                IROpcode::Sub,
                dst_pointer,
                src_pointer,
            }
        );

        this->emit_instruction({ .code = IROpcode::FlagRegisterCheck, .vx = vF, .immediate = 0x55B });
    }

    void IRManager::emit_reg_sub_yx(const Instruction instr) {
        assert(instr.type() == InstructionType::RegSubYX);
        const auto dst = static_cast<IRReg>(instr.used_regs()[0]);
        const auto src = static_cast<IRReg>(instr.used_regs()[1]);

        const auto dst_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(dst) };
        const auto src_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(src) };
        const auto vF = RegisterPointer{ false, this->alloc_temp_for_reg(IRReg::VF) };

        this->emit_instruction({ IROpcode::SubInverse, dst_pointer, src_pointer });
        this->emit_instruction({ .code = IROpcode::FlagRegisterCheck, .vx = vF, .immediate = 0x55B1 });
    }

    void IRManager::emit_reg_shr_xy(const Instruction instr) {
        assert(instr.type() == InstructionType::RegShrXY);
        const auto dst = static_cast<IRReg>(instr.used_regs()[0]);
        const auto src = static_cast<IRReg>(instr.used_regs()[1]);

        const auto dst_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(dst) };
        const auto src_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(src) };
        const auto vF = RegisterPointer{ false, this->alloc_temp_for_reg(IRReg::VF) };

        this->emit_instruction({ IROpcode::ShrOne, dst_pointer, src_pointer });
        this->emit_instruction({ .code = IROpcode::FlagRegisterCheck, .vx = vF, .immediate = 0x5179 });
    }

    void IRManager::emit_reg_shl_xy(const Instruction instr) {
        assert(instr.type() == InstructionType::RegShlXY);
        const auto dst = static_cast<IRReg>(instr.used_regs()[0]);
        const auto src = static_cast<IRReg>(instr.used_regs()[1]);

        const auto dst_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(dst) };
        const auto src_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(src) };
        const auto vF = RegisterPointer{ false, this->alloc_temp_for_reg(IRReg::VF) };

        this->emit_instruction({ IROpcode::ShlOne, dst_pointer, src_pointer });
        this->emit_instruction({ .code = IROpcode::FlagRegisterCheck, .vx = vF, .immediate = 0x5171 });
    }

    void IRManager::emit_range_read(const Instruction instr) {
        assert(instr.type() == InstructionType::RangeRead);
        const auto last_reg = static_cast<IRReg>(instr.used_regs()[0]);

        const auto index_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(IRReg::IN) };

        for (int i = 0; i <= static_cast<int>(last_reg); ++i) {
            const auto vn_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(static_cast<IRReg>(i)) };
            this->emit_instruction(
                { .code = IROpcode::ReadFromMemory,
                  .vx = index_pointer,
                  .vy = vn_pointer,
                  .immediate = static_cast<uint32_t>(i) }
            );
            this->emit_instruction(
                { .code = IROpcode::AddImm, .vx = index_pointer, .vy = index_pointer, .immediate = 1 }
            );
        }
    }

    void IRManager::emit_range_write(const Instruction instr) {
        assert(instr.type() == InstructionType::RangeWrite);
        const auto last_reg = static_cast<IRReg>(instr.used_regs()[0]);

        const auto index_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(last_reg) };

        for (int i = 0; i < static_cast<int>(last_reg); ++i) {
            const auto vn_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(static_cast<IRReg>(i)) };
            this->emit_instruction(
                { .code = IROpcode::WriteToMemory,
                  .vx = index_pointer,
                  .vy = vn_pointer,
                  .immediate = static_cast<uint32_t>(i) }
            );
        }

        this->emit_instruction(
            { .code = IROpcode::AddImm, .vx = index_pointer, .immediate = static_cast<uint32_t>(last_reg) + 1 }
        );
    }

    void IRManager::emit_bcd(const Instruction instr) {
        assert(instr.type() == InstructionType::BCD);
        const auto target_reg = static_cast<IRReg>(instr.used_regs()[0]);
        const auto target_copy_reg = this->new_temp();
        const auto mod_result_reg = this->new_temp();

        const auto target_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(target_reg) };
        const auto index_pointer = RegisterPointer{ false, this->alloc_temp_for_reg(IRReg::IN) };
        const auto target_copy_pointer = RegisterPointer{ true, target_copy_reg };
        const auto mod_result_pointer = RegisterPointer{ true, mod_result_reg };
        const auto mod_scratch_pointer = RegisterPointer{ true, this->new_temp() };

        this->emit_instruction({ IROpcode::LoadReg, target_pointer, target_copy_pointer });
        for (int i = 2; i >= 0; --i) {
            this->emit_instruction(
                { .code = IROpcode::ModImm,
                  .vx = target_copy_pointer,
                  .vy = mod_result_pointer,
                  .immediate = 10,
                  .extra_consumed_registers = { std::pair{ mod_scratch_pointer, RegisterAccessInfo::VYWrite } } }
            );
            this->emit_instruction(
                { IROpcode::WriteToMemory, index_pointer, mod_result_pointer, static_cast<uint32_t>(i) }
            );
            this->emit_instruction({ IROpcode::DivImm, target_copy_pointer, target_copy_pointer, 10 });
        }
    }

    IRManager::BlockHandle IRManager::new_block() noexcept {
        const auto index = this->m_blocks.size();
        this->m_blocks.emplace_back(static_cast<uint16_t>(index));

        return BlockHandle{ this, static_cast<uint16_t>(index) };
    }

    void IRManager::emit_instruction(const IRInstruction& instr) noexcept {
        if (this->m_active_handle.m_owner == nullptr) {
            this->m_active_handle = this->new_block();
            this->m_active_handle.use_block();
        }

        this->m_blocks[this->m_active_handle.m_index].emit_instruction(instr);
    }

    RegisterAccessInfo operator|(const RegisterAccessInfo lhs, const RegisterAccessInfo rhs) {
        return static_cast<RegisterAccessInfo>(std::to_underlying(lhs) | std::to_underlying(rhs));
    }

    RegisterAccessInfo IRManager::access_info(const IRInstruction& code) {
        switch (code.code) {
        case IROpcode::FlagRegisterCheck:
        case IROpcode::LoadImmediate:
        case IROpcode::ReadStackOffset:
            return RegisterAccessInfo::VXWrite;
        case IROpcode::JumpToStackWithOffsetAndDecrement:
            return RegisterAccessInfo::VXWrite | RegisterAccessInfo::VXRead | RegisterAccessInfo::VYWrite;
        case IROpcode::ClearDisplayMemory:
        case IROpcode::JmpBlock:
            return RegisterAccessInfo::None;
        case IROpcode::AddImm:
        case IROpcode::SubImm:
        case IROpcode::AndImm:
        case IROpcode::DivImm:
        case IROpcode::MulImm:
        case IROpcode::ShrImm:
        case IROpcode::ModImm:
        case IROpcode::LoadReg:
        case IROpcode::LoadByteFromI:
        case IROpcode::ReadFromMemory:
            return RegisterAccessInfo::VXRead | RegisterAccessInfo::VYWrite;
        case IROpcode::JmpZ:
        case IROpcode::JmpNZ:
        case IROpcode::JmpEqImm:
        case IROpcode::JmpNeImm:
        case IROpcode::WriteStackOffset:
        case IROpcode::WriteToStackWithOffset:
            return RegisterAccessInfo::VXRead;
        case IROpcode::Add:
        case IROpcode::Sub:
        case IROpcode::SubInverse:
        case IROpcode::OrRegReg:
        case IROpcode::AndRegReg:
        case IROpcode::XorRegReg:
        case IROpcode::ShrOne:
        case IROpcode::ShlOne:
            return RegisterAccessInfo::VXRead | RegisterAccessInfo::VXWrite | RegisterAccessInfo::VYRead;
        case IROpcode::XorDisplayMemory:
        case IROpcode::JmpEqReg:
        case IROpcode::JmpNeReg:
        case IROpcode::WriteToMemory:
            return RegisterAccessInfo::VXRead | RegisterAccessInfo::VYRead;
        default:
            throw std::logic_error("Unhandled instruction Info");
        }
    }
} // namespace jip
