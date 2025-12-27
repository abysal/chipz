#include "jit_manager.hpp"

#include "cpu/chip_core.hpp"
#include "instruction_list.hpp"
#include "linear_register_allocator.hpp"
#include "util/division.hpp"

#include <algorithm>
#include <asmjit/x86.h>
#include <format>

#include <memory>
#include <print>

namespace jip {
    using namespace asmjit::x86;
    constexpr static auto arg_1 = rdi;

    static JitManager* raw_instance = nullptr;

    Gp remap_8_16(const Gp& reg_8) noexcept {
        const static auto register_remapping_8_16 = std::vector<std::pair<Gp, Gp>>{
            { al,   ax   },
            { bl,   bx   },
            { cl,   cx   },
            { dl,   dx   },
            { sil,  si   },
            { dil,  di   },
            { r8b,  r8w  },
            { r9b,  r9w  },
            { r10b, r10w },
            { r11b, r11w },
            { r12b, r12w },
            { r13b, r13w },
            { r14b, r14w },
            { r15b, r15w }
        };
        for (const auto& [bit_8, bit_16] : register_remapping_8_16) {
            if (bit_8 == reg_8) {
                return bit_16;
            }
        }

        std::unreachable();
    }

    Gp remap_8_64(const Gp& reg_8) noexcept {
        const static auto register_remapping_8_64 = std::vector<std::pair<Gp, Gp>>{
            { al,   rax },
            { bl,   rbx },
            { bpl,  rbp },
            { cl,   rcx },
            { dl,   rdx },
            { sil,  rsi },
            { dil,  rdi },
            { r8b,  r8  },
            { r9b,  r9  },
            { r10b, r10 },
            { r11b, r11 },
            { r12b, r12 },
            { r13b, r13 },
            { r14b, r14 },
            { r15b, r15 }
        };

        for (const auto& [bit_8, bit_64] : register_remapping_8_64) {
            if (bit_8 == reg_8) {
                return bit_64;
            }
        }

        std::unreachable();
    }

    Gp remap_16_32(const Gp& reg_16) noexcept {
        const static auto register_remapping_16_32 = std::vector<std::pair<Gp, Gp>>{
            { ax,   eax  },
            { bx,   ebx  },
            { cx,   ecx  },
            { dx,   edx  },
            { si,   esi  },
            { di,   edi  },
            { bp,   ebp  },
            { sp,   esp  },
            { r8w,  r8d  },
            { r9w,  r9d  },
            { r10w, r10d },
            { r11w, r11d },
            { r12w, r12d },
            { r13w, r13d },
            { r14w, r14d },
            { r15w, r15d }
        };
        for (const auto& [bit_16, bit_32] : register_remapping_16_32) {
            if (bit_16 == reg_16) {
                return bit_32;
            }
        }
        std::unreachable();
    }

    Gp remap_8_32(const Gp& reg_8) noexcept {
        const static auto register_remapping_8_32 = std::vector<std::pair<Gp, Gp>>{
            { al,   eax  },
            { bl,   ebx  },
            { cl,   ecx  },
            { dl,   edx  },
            { sil,  esi  },
            { dil,  edi  },
            { bpl,  ebp  },
            { spl,  esp  },
            { r8b,  r8d  },
            { r9b,  r9d  },
            { r10b, r10d },
            { r11b, r11d },
            { r12b, r12d },
            { r13b, r13d },
            { r14b, r14d },
            { r15b, r15d }
        };
        for (const auto& [bit_8, bit_32] : register_remapping_8_32) {
            if (bit_8 == reg_8) {
                return bit_32;
            }
        }
        std::unreachable();
    }

    JitManager::JitManager() { raw_instance = this; }

    JitBlock JitManager::compile_block(const uint16_t current_ip, const MemoryStream& block_memory) noexcept {
        InstructionList chip_instrs{};
        chip_instrs.create_block(block_memory, current_ip);

        auto ir = emit_ir(chip_instrs, current_ip);
        LinearRegisterAllocator reg_allocator{};
        reg_allocator.track(*ir);

        const auto compiler = std::make_unique<BlockCompiler>(this, std::move(ir), std::move(reg_allocator));
        compiler->emit_machine_code(current_ip);

        return BlockCompiler::as_jit_block(compiler);
    }

    [[noreturn]] void JitManager::execute_loop(const uint16_t start_ip, const JitBlock start_block) noexcept {
        this->m_blocks.emplace(std::pair{ start_ip, start_block });

        this->m_execution_block = &this->m_blocks.at(start_ip);
        while (true) {
            const auto next_address = this->m_execution_block->execute();

            auto it = this->m_blocks.find(next_address);

            if (it == this->m_blocks.end()) {
                auto [new_it, inserted] = this->m_blocks.emplace(
                    next_address,
                    this->compile_block(
                        next_address, MemoryStream{ std::span{ this->m_core_state->memory }.subspan(next_address) }
                    )
                );
                it = new_it;
            }

            this->m_execution_block = &it->second;
        }
    }

    std::unique_ptr<IRManager>
    JitManager::emit_ir(const InstructionList& instructions, const uint16_t start_ip) noexcept {
        auto ir_manager = std::make_unique<IRManager>();
        ir_manager->init_jump_points(instructions.jump_points());

        for (const auto& [index, instr] : instructions | std::views::enumerate) {
            ir_manager->emit(instr, static_cast<uint16_t>(index) * 2 + start_ip);
        }

        return ir_manager;
    }

    JitManager::BlockCompiler::BlockCompiler(
        JitManager* manager, std::unique_ptr<IRManager> ir, LinearRegisterAllocator register_allocator
    ) noexcept {
        this->m_manager = manager;
        this->m_ir = std::move(ir);
        this->m_register_allocator = std::move(register_allocator);
        this->m_code.init(this->m_manager->m_rt.environment(), this->m_manager->m_rt.cpu_features());
        this->m_code.attach(&this->m_builder);

        for ([[maybe_unused]] const auto& block : this->m_ir->blocks()) {
            this->m_block_labels.emplace_back(this->m_builder.new_label());
        }

        using namespace asmjit::x86;
        this->m_register_allocator.initialize_free_regs(
            { r15b, r14b, r13b, r12b, r11b, r10b, r9b, r8b, dil, sil, dl, cl, bl, al }
        );

        this->m_register_allocator.initialize_clobber_aware_registers({ bl, bpl, r12b, r13b, r14b, r15b });
    }

    void JitManager::BlockCompiler::emit_machine_code(const uint16_t ip) {
        auto& a = this->m_builder;
        auto* original_prev = a.cursor()->prev();

        uint32_t current_ip{ 0 };
        cip::StaticVector<LinearRegisterAllocator::UsedRegInfo, GPRegCount, uint8_t> regs_to_store{};
        this->m_register_allocator.add_clobbered(bpl);

        for (const auto& block : this->m_ir->blocks()) {
            auto& label = this->label_for_block(block);

            a.bind(label);

            for (const auto& instr : block.instructions()) {
                this->m_register_allocator.free_if_possible(
                    current_ip, this->m_spill_mapping, this->m_spill_free_offsets, regs_to_store
                );

                if (!regs_to_store.empty()) {
                    for (const auto& reg : regs_to_store) {
                        this->emit_register_backup(reg);
                    }
                }
                regs_to_store.clear();

                this->compile_instruction(instr, current_ip);

                current_ip++;
            }
        }

        for (auto* node : this->m_restore_locations) {
            a.set_cursor(node);
            this->emit_clobber_restore();
        }

        a.set_cursor(original_prev);

        const auto clobbered = this->m_register_allocator.clobbered_regs();

        for (const auto& reg : clobbered) {
            a.push(remap_8_64(reg));
        }

        a.sub(StackPointer, this->m_last_spill_offset);
        a.mov(CoreStatePointer, std::bit_cast<uintptr_t>(this->m_manager->m_core_state));

        asmjit::String str{};
        constexpr asmjit::FormatOptions opts{};
        asmjit::Formatter::format_node_list(str, opts, &this->m_builder);
        std::println("{}", std::string{ str.data() });
    }

    JitBlock JitManager::BlockCompiler::as_jit_block(const std::unique_ptr<BlockCompiler>& compiler) {
        auto error = compiler->m_builder.finalize();

        if (error == asmjit::Error::kOk) {
            void* memory;
            std::println("Block Size: 0x{:x}", compiler->m_code.code_size());
            error = compiler->m_manager->m_rt.add(&memory, &compiler->m_code);

            if (error == asmjit::Error::kOk) {
                return JitBlock{ memory };
            }
        }

        throw std::runtime_error(std::format("Failed to compile JIT block: {}", static_cast<uint32_t>(error)));
    }

    void JitManager::BlockCompiler::compile_instruction(const IRInstruction& instruction, const uint32_t current_ip) {
        switch (instruction.code) {
        case IROpcode::LoadImmediate:
            this->compile_load_imm(instruction, current_ip);
            return;
        case IROpcode::AddImm:
            this->compile_add_imm(instruction, current_ip);
            return;
        case IROpcode::FlagRegisterCheck:
            this->compile_flag_reg_handler(instruction, current_ip);
            return;
        case IROpcode::AndImm:
            this->compile_and_imm(instruction, current_ip);
            return;
        case IROpcode::LoadByteFromI:
            this->compile_load_byte_index_reg(instruction, current_ip);
            return;
        case IROpcode::JmpZ:
            this->compile_jmpz(instruction, current_ip);
            return;
        case IROpcode::JmpNZ:
            this->compile_jmpnz(instruction, current_ip);
            return;
        case IROpcode::JmpEqImm:
            this->compile_jump_eq_imm(instruction, current_ip);
            return;
        case IROpcode::JmpNeImm:
            this->compile_jump_ne_imm(instruction, current_ip);
            return;
        case IROpcode::JmpEqReg:
            this->compile_jump_eq_reg(instruction, current_ip);
            return;
        case IROpcode::JmpNeReg:
            this->compile_jump_ne_reg(instruction, current_ip);
            return;
        case IROpcode::XorDisplayMemory:
            this->compile_xor_display_memory(instruction, current_ip);
            return;
        case IROpcode::ClearDisplayMemory:
            this->compile_native_clear(instruction, current_ip);
            return;
        case IROpcode::JmpBlock:
            this->compile_jump_block(instruction, current_ip);
            return;
        case IROpcode::JmpJit:
            this->compile_jump_jit(instruction, current_ip);
            return;
        case IROpcode::ReadStackOffset:
            this->compile_read_stack_offset(instruction, current_ip);
            return;
        case IROpcode::WriteStackOffset:
            this->compile_write_stack_offset(instruction, current_ip);
            return;
        case IROpcode::WriteToStackWithOffset:
            this->compile_write_to_stack_with_offset(instruction, current_ip);
            return;
        case IROpcode::JumpToStackWithOffsetAndDecrement:
            this->compile_jump_to_stack_with_offset_and_decrement(instruction, current_ip);
            return;
        case IROpcode::Add:
            this->compile_add(instruction, current_ip);
            return;
        case IROpcode::Sub:
            this->compile_sub(instruction, current_ip);
            return;
        case IROpcode::SubInverse:
            this->compile_sub_inverse(instruction, current_ip);
            return;
        case IROpcode::OrRegReg:
            this->compile_or_reg_reg(instruction, current_ip);
            return;
        case IROpcode::AndRegReg:
            this->compile_and_reg_reg(instruction, current_ip);
            return;
        case IROpcode::XorRegReg:
            this->compile_xor_reg_reg(instruction, current_ip);
            return;
        case IROpcode::ShrOne:
            this->compile_shr_one(instruction, current_ip);
            return;
        case IROpcode::ShlOne:
            this->compile_shl_one(instruction, current_ip);
            return;
        case IROpcode::DivImm:
            this->compile_div_imm(instruction, current_ip);
            return;
        case IROpcode::ModImm:
            this->compile_mod_imm(instruction, current_ip);
            return;
        case IROpcode::ReadFromMemory:
            this->compile_read_from_memory(instruction, current_ip);
            return;
        default:
            std::println("Unknown instruction!: {:x}", static_cast<uint32_t>(instruction.code));
            // throw std::runtime_error("Unhandled instruction code");
        }
    }

    void
    JitManager::BlockCompiler::compile_add_imm(const IRInstruction& instruction, const uint32_t current_ip) noexcept {
        auto& a = this->m_builder;
        const auto source_reg = this->get_reg(*instruction.vx, current_ip);
        const auto dest_reg = this->get_reg(*instruction.vy, current_ip);

        if (source_reg != dest_reg) {
            a.mov(dest_reg, source_reg);
        }

        const auto imm = instruction.immediate;
        if (imm == 0) {
            return;
        }

        if (imm == 1) {
            a.inc(dest_reg);
            return;
        }

        a.add(dest_reg, imm);
    }

    void JitManager::BlockCompiler::compile_add(const IRInstruction& instruction, const uint32_t current_ip) {
        auto& a = this->m_builder;
        const auto vx = this->get_reg(*instruction.vx, current_ip);
        const auto vy = this->get_reg(*instruction.vy, current_ip);

        a.add(vx, vy);
    }

    void JitManager::BlockCompiler::compile_sub(const IRInstruction& instruction, const uint32_t current_ip) {
        auto& a = this->m_builder;
        const auto vx = this->get_reg(*instruction.vx, current_ip);
        const auto vy = this->get_reg(*instruction.vy, current_ip);

        a.xchg(vx, vy);
        a.sub(vx, vy);
        a.xchg(vx, vy);
    }

    void JitManager::BlockCompiler::compile_sub_inverse(const IRInstruction& instruction, const uint32_t current_ip) {
        auto& a = this->m_builder;
        const auto vx = this->get_reg(*instruction.vx, current_ip);
        const auto vy = this->get_reg(*instruction.vy, current_ip);

        a.sub(vx, vy);
    }

    void
    JitManager::BlockCompiler::compile_load_imm(const IRInstruction& instruction, const uint32_t current_ip) noexcept {
        auto& a = this->m_builder;
        const auto dst = this->get_reg(*instruction.vx, current_ip);

        if (instruction.immediate == 0) {
            const auto remapped = remap_8_32(dst);
            a.xor_(remapped, remapped); // faster than mov 0
        } else {
            a.mov(dst, instruction.immediate);
        }
    }

    void
    JitManager::BlockCompiler::compile_and_imm(const IRInstruction& instruction, const uint32_t current_ip) noexcept {
        auto& a = this->m_builder;
        const auto dst = this->get_reg(*instruction.vy, current_ip);
        const auto src = this->get_reg(*instruction.vx, current_ip);

        if (dst != src) {
            a.mov(dst, src);
        }

        a.and_(dst, instruction.immediate);
    }

    void JitManager::BlockCompiler::compile_and_reg_reg(const IRInstruction& instruction, const uint32_t current_ip) {
        auto& a = this->m_builder;
        const auto vx = this->get_reg(*instruction.vx, current_ip);
        const auto vy = this->get_reg(*instruction.vy, current_ip);

        a.and_(vx, vy);
    }
    void JitManager::BlockCompiler::compile_xor_reg_reg(const IRInstruction& instruction, const uint32_t current_ip) {
        auto& a = this->m_builder;
        const auto vx = this->get_reg(*instruction.vx, current_ip);
        const auto vy = this->get_reg(*instruction.vy, current_ip);

        a.xor_(vx, vy);
    }

    void JitManager::BlockCompiler::compile_or_reg_reg(const IRInstruction& instruction, const uint32_t current_ip) {
        auto& a = this->m_builder;
        const auto vx = this->get_reg(*instruction.vx, current_ip);
        const auto vy = this->get_reg(*instruction.vy, current_ip);

        a.or_(vx, vy);
    }

    void JitManager::BlockCompiler::compile_flag_reg_handler(
        const IRInstruction& instruction, const uint32_t current_ip
    ) noexcept {
        assert(instruction.vx->is_temp == false);
        const auto reg = instruction.vx->reg;
        const auto operation = instruction.immediate;

        if (this->m_register_allocator.next_access_is_write_only(reg, current_ip)) {
            return;
            // We only need to maintain the flag register if its used
        }

        auto& a = this->m_builder;
        const auto flag_reg = this->get_reg(*instruction.vx, current_ip);

        if (operation == 0xADD || operation == 0x55B || operation == 0x5179 || operation == 0x5171) {
            a.setc(flag_reg);
            return;
        }

        if (operation == 0x55B1) {
            a.setc(flag_reg);
            a.xor_(flag_reg, 1);
            return;
        }

        throw std::logic_error("Unhandled flag operation");
    }

    void JitManager::BlockCompiler::compile_load_byte_index_reg(
        const IRInstruction& instruction, const uint32_t current_ip
    ) noexcept {
        auto& a = this->m_builder;
        const auto index_reg = this->get_reg(*instruction.vx, current_ip);
        const auto result = this->get_reg(*instruction.vy, current_ip);
        const auto offset = instruction.immediate;
        const auto index_32bit = remap_16_32(index_reg);

        a.movzx(index_32bit, index_reg);
        a.mov(
            result, byte_ptr(CoreStatePointer, index_reg, 0, static_cast<int32_t>(offset + offsetof(CoreState, memory)))
        );
    }

    void JitManager::BlockCompiler::compile_jmpz(const IRInstruction& instruction, const uint32_t current_ip) noexcept {
        auto& a = this->m_builder;
        const auto src = this->get_reg(*instruction.vx, current_ip);

        a.test(src, src);
        a.jz(this->label_for_block(instruction.immediate));
    }

    void
    JitManager::BlockCompiler::compile_jmpnz(const IRInstruction& instruction, const uint32_t current_ip) noexcept {
        auto& a = this->m_builder;
        const auto src = this->get_reg(*instruction.vx, current_ip);

        a.test(src, src);
        a.jnz(this->label_for_block(instruction.immediate));
    }

    void JitManager::BlockCompiler::compile_xor_display_memory(
        const IRInstruction& instruction, const uint32_t current_ip
    ) noexcept {
        auto& a = this->m_builder;
        const auto x = this->get_reg(*instruction.vx, current_ip);
        const auto y = this->get_reg(*instruction.vy, current_ip);
        const auto x32 = remap_8_32(x);
        const auto y32 = remap_8_32(y);

        a.movzx(y32, y); // Zero extends the register Y is in to be 32bits which is needed for byte pointers
        a.movzx(x32, x);
        a.push(y32);
        a.shl(y32, 6);
        a.add(y32, x32);
        a.xor_(byte_ptr(CoreStatePointer, y32, 0, offsetof(CoreState, core_display)), 1);
        a.pop(y32);
    }

    void
    JitManager::BlockCompiler::compile_shr_imm(const IRInstruction& instruction, const uint32_t current_ip) noexcept {
        auto& a = this->m_builder;
        const auto src = this->get_reg(*instruction.vx, current_ip);
        const auto dst = this->get_reg(*instruction.vy, current_ip);

        if (dst != src) {
            a.mov(dst, src);
        }
        a.shr(dst, instruction.immediate);
    }

    void JitManager::BlockCompiler::compile_shr_one(const IRInstruction& instruction, const uint32_t uint32) {
        auto& a = this->m_builder;
        const auto vx = this->get_reg(*instruction.vx, uint32);
        const auto vy = this->get_reg(*instruction.vy, uint32);

        if (vx != vy) {
            a.mov(vx, vy);
        }

        a.shr(vx, 1);
    }
    void JitManager::BlockCompiler::compile_shl_one(const IRInstruction& instruction, uint32_t current_ip) {
        auto& a = this->m_builder;
        const auto vx = this->get_reg(*instruction.vx, current_ip);
        const auto vy = this->get_reg(*instruction.vy, current_ip);

        if (vx != vy) {
            a.mov(vx, vy);
        }

        a.shl(vx, 1);
    }

    void
    JitManager::BlockCompiler::compile_load_reg(const IRInstruction& instruction, const uint32_t current_ip) noexcept {
        auto& a = this->m_builder;
        const auto src = this->get_reg(*instruction.vx, current_ip);
        const auto dst = this->get_reg(*instruction.vy, current_ip);

        if (dst != src) {
            a.mov(dst, src);
        }
    }

    void JitManager::BlockCompiler::compile_native_clear(const IRInstruction&, uint32_t) noexcept {
        auto& a = this->m_builder;

        a.vpxor(ymm0, ymm0, ymm0);
        for (int i = 0; i < 2048 / 32; i++) {
            a.vmovdqa(ymmword_ptr(CoreStatePointer, offsetof(CoreState, core_display) + i * 32), ymm0);
        }
    }

    void JitManager::BlockCompiler::compile_jump_block(const IRInstruction& instruction, uint32_t) noexcept {
        auto& a = this->m_builder;
        a.jmp(label_for_block(instruction.immediate));
    }

    void JitManager::BlockCompiler::compile_jump_eq_imm(
        const IRInstruction& instruction, const uint32_t current_ip
    ) noexcept {
        auto& a = this->m_builder;

        const auto src = this->get_reg(*instruction.vx, current_ip);

        a.cmp(src, instruction.immediate);
        a.je(this->label_for_block(instruction.immediate_2));
    }

    void JitManager::BlockCompiler::compile_jump_ne_imm(
        const IRInstruction& instruction, const uint32_t current_ip
    ) noexcept {
        auto& a = this->m_builder;

        const auto src = this->get_reg(*instruction.vx, current_ip);

        a.cmp(src, instruction.immediate);
        a.jne(this->label_for_block(instruction.immediate_2));
    }

    void JitManager::BlockCompiler::compile_jump_eq_reg(
        const IRInstruction& instruction, const uint32_t current_ip
    ) noexcept {
        auto& a = this->m_builder;

        const auto src = this->get_reg(*instruction.vx, current_ip);
        const auto src2 = this->get_reg(*instruction.vy, current_ip);

        a.cmp(src, src2);
        a.je(this->label_for_block(instruction.immediate));
    }

    void JitManager::BlockCompiler::compile_jump_ne_reg(
        const IRInstruction& instruction, const uint32_t current_ip
    ) noexcept {
        auto& a = this->m_builder;

        const auto src = this->get_reg(*instruction.vx, current_ip);
        const auto src2 = this->get_reg(*instruction.vy, current_ip);

        a.cmp(src, src2);
        a.jne(this->label_for_block(instruction.immediate));
    }

    void JitManager::BlockCompiler::compile_jump_jit(const IRInstruction& instruction, uint32_t) noexcept {
        using namespace asmjit;
        auto& a = this->m_builder;
        const auto target_ip = instruction.immediate;

        this->emit_register_saves();
        this->add_clobber_restore_point();
        // Stack is already aligned :D

        a.mov(rax, target_ip);
        this->emit_stack_alignment_check();
        a.ret();
    }

    void JitManager::BlockCompiler::compile_read_stack_offset(
        const IRInstruction& instruction, const uint32_t current_ip
    ) noexcept {
        auto& a = this->m_builder;

        const auto dst = this->get_reg(*instruction.vx, current_ip);
        a.mov(dst, byte_ptr(CoreStatePointer, offsetof(CoreState, stack) + StackType::offset_of_size()));
    }

    void JitManager::BlockCompiler::compile_write_stack_offset(
        const IRInstruction& instruction, const uint32_t current_ip
    ) noexcept {
        auto& a = this->m_builder;

        const auto dst = this->get_reg(*instruction.vx, current_ip);
        a.mov(byte_ptr(CoreStatePointer, offsetof(CoreState, stack) + StackType::offset_of_size()), dst);
    }

    static_assert(StackType::offset_of_size() == 32);

    void JitManager::BlockCompiler::compile_write_to_stack_with_offset(
        const IRInstruction& instruction, const uint32_t current_ip
    ) noexcept {
        auto& a = this->m_builder;
        const auto offset_reg = this->get_reg(*instruction.vx, current_ip);
        const auto offset_reg_32 = remap_8_32(offset_reg);
        const auto value = instruction.immediate;

        a.movzx(offset_reg_32, offset_reg);
        a.mov(
            word_ptr(CoreStatePointer, offset_reg_32, 1, offsetof(CoreState, stack) + StackType::offset_of_storage()),
            value
        );
    }

    void JitManager::BlockCompiler::compile_jump_to_stack_with_offset_and_decrement(
        const IRInstruction& instruction, const uint32_t current_ip
    ) noexcept {
        auto& a = this->m_builder;
        const auto small_offset = this->get_reg(*instruction.vx, current_ip);
        const auto return_scratch = remap_8_16(this->get_reg(*instruction.vy, current_ip));
        const auto offset = remap_8_32(small_offset);
        this->emit_register_saves(); // we have to emit here, else we risk overriding the value when we move into rax
        // this is a termination point so data isn't needed and can be thrashed from this
        // point on

        a.movzx(offset, small_offset);
        a.mov(
            return_scratch,
            word_ptr(CoreStatePointer, offset, 1, offsetof(CoreState, stack) + StackType::offset_of_storage())
        );
        a.dec(offset);
        a.mov(byte_ptr(CoreStatePointer, offsetof(CoreState, stack) + StackType::offset_of_size()), small_offset);

        a.movzx(rax, return_scratch);
        // Stack is already aligned :D
        this->add_clobber_restore_point();
        this->emit_stack_alignment_check();
        a.ret();
    }

    void JitManager::BlockCompiler::compile_div_imm(const IRInstruction& instruction, const uint32_t current_ip) {
        auto& a = this->m_builder;

        const auto src = this->get_reg(*instruction.vx, current_ip);
        const auto dst = this->get_reg(*instruction.vy, current_ip);
        const auto divisor = instruction.immediate;

        this->emit_mov(src, dst);

        if (divisor == 1) {
            return;
        }

        if (std::popcount(divisor) == 1) {
            const auto shift_count = std::countr_zero(divisor);
            a.shr(dst, shift_count);
        } else {
            const auto magic = compute_magic_for_d(divisor);
            const auto large_dst = remap_8_32(dst);

            a.movzx(large_dst, dst);
            if (magic.mul != 1) {
                a.imul(large_dst, large_dst, magic.mul);
            }

            if (magic.shift != 0) {
                a.shr(large_dst, magic.shift);
            }
        }
    }

    void JitManager::BlockCompiler::compile_mod_imm(const IRInstruction& instruction, uint32_t uint32) {
        auto& a = this->m_builder;

        const auto src = this->get_reg(*instruction.vx, uint32);
        const auto dst = this->get_reg(*instruction.vy, uint32);
        const auto scratch = remap_8_32(this->get_reg(instruction.extra_consumed_registers[0].first, uint32));
        const auto divisor = instruction.immediate;

        this->emit_mov(src, dst);

        if (divisor == 1) {
            const auto extended = remap_8_32(dst);
            a.xor_(extended, extended);
            return;
        }

        if (std::popcount(divisor) == 1) {
            a.and_(dst, divisor - 1);
            return;
        }

        const auto magic = compute_magic_for_d(divisor);
        const auto large_dst = remap_8_32(dst);

        a.movzx(scratch, src);
        a.movzx(large_dst, dst);
        if (magic.mul != 1) {
            a.imul(large_dst, large_dst, magic.mul);
        }

        if (magic.shift != 0) {
            a.shr(large_dst, magic.shift);
        }

        a.imul(large_dst, large_dst, divisor);
        a.xchg(scratch, large_dst);
        a.sub(large_dst, scratch);
    }

    void JitManager::BlockCompiler::compile_read_from_memory(const IRInstruction& instruction, uint32_t uint32) {
        auto& a = this->m_builder;

        const auto index = this->get_reg(*instruction.vx, uint32);
        const auto dst = this->get_reg(*instruction.vy, uint32);
    }

    asmjit::Label& JitManager::BlockCompiler::label_for_block(const IRManager::IRBlock& block) noexcept {
        return this->m_block_labels[block.block_id()];
    }

    asmjit::Label& JitManager::BlockCompiler::label_for_block(const uint16_t block_id) noexcept {
        return this->m_block_labels[this->m_ir->blocks()[block_id].block_id()];
    }

    RegType JitManager::BlockCompiler::get_reg(const RegisterPointer pointer, const uint32_t rel_ip) noexcept {
        const auto action = this->m_register_allocator.allocate(pointer.reg, rel_ip);
        auto& a = this->m_builder;

        if ((action.actions & LinearRegisterAllocator::Actions::Spill) == LinearRegisterAllocator::Actions::Spill) {
            const auto spill_register = action.spill_info.register_index;
            const auto reg = this->m_register_allocator.get_ir_reg(spill_register);

            if (reg == IRReg::Invalid) {
                const auto offset_from_stack = this->get_spill_offset_for_temp_reg(spill_register);
                a.mov(dword_ptr(StackPointer, static_cast<int32_t>(offset_from_stack)), remap_8_32(action.reg_type));
            } else if (static_cast<uint16_t>(reg) < 16) {
                a.mov(byte_ptr(CoreStatePointer, static_cast<int32_t>(reg)), action.reg_type);
            } else if (reg == IRReg::IN) {
                a.mov(word_ptr(CoreStatePointer, static_cast<int32_t>(reg)), remap_8_16(action.reg_type));
            } else {
                throw std::logic_error("Unhandled register");
            }
        }

        if ((action.actions & LinearRegisterAllocator::Actions::Load) == LinearRegisterAllocator::Actions::Load &&
            pointer.is_temp) {
            a.mov(
                remap_8_32(action.reg_type),
                dword_ptr(StackPointer, static_cast<int32_t>(this->m_spill_mapping.at(pointer.reg)))
            );
        } else if ((action.actions & LinearRegisterAllocator::Actions::Load) ==
                       LinearRegisterAllocator::Actions::Load &&
                   !pointer.is_temp) {
            const auto offset = this->m_register_allocator.get_ir_reg(pointer.reg);

            if (static_cast<uint16_t>(offset) < 16) {
                a.mov(action.reg_type, byte_ptr(CoreStatePointer, static_cast<uint16_t>(offset)));
            } else if (offset == IRReg::IN) {
                const auto remapped = remap_8_16(action.reg_type);
                a.mov(remapped, word_ptr(CoreStatePointer, static_cast<uint16_t>(offset)));
            } else {
                throw std::logic_error("Unhandled register");
            }
        }

        const auto offset = this->m_register_allocator.get_ir_reg(pointer.reg);
        if (offset == IRReg::IN) {
            return remap_8_16(action.reg_type);
        }

        return action.reg_type;
    }

    uint32_t JitManager::BlockCompiler::get_spill_offset_for_temp_reg(const uint32_t reg) noexcept {
        if (this->m_spill_mapping.contains(reg)) {
            return this->m_spill_mapping.at(reg);
        }

        if (!this->m_spill_free_offsets.empty()) {
            const auto offset = this->m_spill_free_offsets.back();
            this->m_spill_free_offsets.pop_back();
            this->m_spill_mapping[reg] = offset;
            return offset;
        }

        this->m_spill_mapping[reg] = this->m_last_spill_offset;
        this->m_last_spill_offset += 4;
        return this->m_last_spill_offset - 4;
    }

    void JitManager::BlockCompiler::emit_register_backup(const LinearRegisterAllocator::UsedRegInfo& reg) {
        const auto reg_type = this->m_register_allocator.get_ir_reg(reg.reg_index);
        auto& a = this->m_builder;
        if (reg_type == IRReg::Invalid) {
            return;
        }

        if (static_cast<uint32_t>(reg_type) < static_cast<uint32_t>(IRReg::IN)) {
            a.mov(byte_ptr(CoreStatePointer, static_cast<int32_t>(reg_type)), reg.allocated_register);
        } else if (reg_type == IRReg::IN) {
            a.mov(dword_ptr(CoreStatePointer, static_cast<int32_t>(reg_type)), remap_8_16(reg.allocated_register));
        } else {
            throw std::logic_error("Unhandled register");
        }
    }

    void JitManager::BlockCompiler::emit_register_saves() noexcept {
        auto& a = this->m_builder;

        for (const auto& reg : this->m_register_allocator.allocated_regs()) {
            emit_register_backup(reg);
        }

        if (this->m_last_spill_offset != 0) {
            a.add(StackPointer, this->m_last_spill_offset);
        }
    }

    // Clobbers RBP
    void JitManager::BlockCompiler::emit_stack_alignment_check() noexcept {
        auto& a = this->m_builder;
        const auto skip = a.new_label();

        a.mov(r11, StackPointer);
        a.sub(r11, 8);
        a.and_(r11, 0xF);
        a.test(r11, r11);
        a.jz(skip);
        a.int3();
        a.bind(skip);
    }

    void JitManager::BlockCompiler::emit_clobber_restore() noexcept {
        auto& a = this->m_builder;

        const auto& clobbered = this->m_register_allocator.clobbered_regs();
        for (const auto& reg : clobbered | std::ranges::views::reverse) {
            a.pop(remap_8_64(reg));
        }
    }
    void JitManager::BlockCompiler::add_clobber_restore_point() noexcept {
        auto& a = this->m_builder;

        auto* const current_node = a.cursor();
        this->m_restore_locations.emplace_back(current_node);
    }

    void JitManager::BlockCompiler::emit_mov(const RegType& src, const RegType& dst) noexcept {
        auto& a = this->m_builder;
        if (src != dst) {
            a.mov(dst, src);
        }
    }
} // namespace jip
