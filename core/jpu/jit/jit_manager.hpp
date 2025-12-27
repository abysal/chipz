#pragma once
#include "asmjit/core/jitruntime.h"
#include "instruction_list.hpp"
#include "ir/ir_manager.hpp"
#include "jit_block.hpp"
#include "jpu/core.hpp"
#include "linear_register_allocator.hpp"
#include "util/memory_stream.hpp"
#include <asmjit/x86.h>

#include <memory>
#include <unordered_map>

namespace jip {
    constexpr static auto TotalRegCount = 16 + 2;

    class JitManager {
    public:
        JitManager();
        JitManager(const JitManager&) = delete;
        JitManager(JitManager&&) = delete;
        JitManager& operator=(const JitManager&) = delete;
        JitManager& operator=(JitManager&&) = delete;
        ~JitManager() = default;

        void set_state(CoreState* state) noexcept { this->m_core_state = state; }

        JitBlock compile_block(uint16_t current_ip, const MemoryStream& block_memory) noexcept;

        [[noreturn]] void execute_loop(uint16_t start_ip, JitBlock start_block) noexcept;

    private:
        [[nodiscard]] static std::unique_ptr<IRManager>
        emit_ir(const InstructionList& instructions, uint16_t start_ip) noexcept;

        class BlockCompiler {
        public:
            BlockCompiler(
                JitManager* manager, std::unique_ptr<IRManager> ir, LinearRegisterAllocator register_allocator
            ) noexcept;

            void emit_machine_code(uint16_t ip);

            static JitBlock as_jit_block(const std::unique_ptr<BlockCompiler>& compiler);

        private:
            void compile_instruction(const IRInstruction& instruction, uint32_t current_ip);

            void compile_add_imm(const IRInstruction& instruction, uint32_t current_ip) noexcept;
            void compile_add(const IRInstruction& instruction, uint32_t current_ip);
            void compile_sub(const IRInstruction& instruction, uint32_t current_ip);
            void compile_sub_inverse(const IRInstruction& instruction, uint32_t current_ip);
            void compile_load_imm(const IRInstruction& instruction, uint32_t current_ip) noexcept;
            void compile_or_reg_reg(const IRInstruction& instruction, uint32_t current_ip);
            void compile_and_imm(const IRInstruction& instruction, uint32_t current_ip) noexcept;
            void compile_and_reg_reg(const IRInstruction& instruction, uint32_t current_ip);
            void compile_xor_reg_reg(const IRInstruction& instruction, uint32_t current_ip);
            void compile_flag_reg_handler(const IRInstruction& instruction, uint32_t current_ip) noexcept;
            void compile_load_byte_index_reg(const IRInstruction& instruction, uint32_t current_ip) noexcept;
            void compile_jmpz(const IRInstruction& instruction, uint32_t current_ip) noexcept;
            void compile_jmpnz(const IRInstruction& instruction, uint32_t current_ip) noexcept;
            void compile_xor_display_memory(const IRInstruction& instruction, uint32_t current_ip) noexcept;
            void compile_shr_imm(const IRInstruction& instruction, uint32_t current_ip) noexcept;
            void compile_shr_one(const IRInstruction& instruction, uint32_t current_ip);
            void compile_shl_one(const IRInstruction& instruction, uint32_t current_ip);
            void compile_load_reg(const IRInstruction& instruction, uint32_t current_ip) noexcept;
            void compile_native_clear(const IRInstruction& instruction, uint32_t current_ip) noexcept;
            void compile_jump_block(const IRInstruction& instruction, uint32_t current_ip) noexcept;
            void compile_jump_eq_imm(const IRInstruction& instruction, uint32_t current_ip) noexcept;
            void compile_jump_ne_imm(const IRInstruction& instruction, uint32_t current_ip) noexcept;
            void compile_jump_eq_reg(const IRInstruction& instruction, uint32_t current_ip) noexcept;
            void compile_jump_ne_reg(const IRInstruction& instruction, uint32_t current_ip) noexcept;
            void compile_jump_jit(const IRInstruction& instruction, uint32_t current_ip) noexcept;
            void compile_read_stack_offset(const IRInstruction& instruction, uint32_t current_ip) noexcept;
            void compile_write_stack_offset(const IRInstruction& instruction, uint32_t current_ip) noexcept;
            void compile_write_to_stack_with_offset(const IRInstruction& instruction, uint32_t current_ip) noexcept;
            void compile_jump_to_stack_with_offset_and_decrement(
                const IRInstruction& instruction, uint32_t current_ip
            ) noexcept;
            void compile_div_imm(const IRInstruction& instruction, uint32_t current_ip);
            void compile_mod_imm(const IRInstruction& instruction, uint32_t uint32);
            void compile_read_from_memory(const IRInstruction& instruction, uint32_t uint32);

            asmjit::Label& label_for_block(const IRManager::IRBlock& block) noexcept;
            asmjit::Label& label_for_block(uint16_t block_id) noexcept;

            RegType get_reg(RegisterPointer pointer, uint32_t rel_ip) noexcept;

            uint32_t get_spill_offset_for_temp_reg(uint32_t reg) noexcept;
            void emit_register_backup(const LinearRegisterAllocator::UsedRegInfo& reg);

            void emit_register_saves() noexcept;
            void emit_stack_alignment_check() noexcept;
            void emit_clobber_restore() noexcept;
            void add_clobber_restore_point() noexcept;

            void emit_mov(const RegType& src, const RegType& dst) noexcept;

        private:
            asmjit::x86::Builder m_builder{};
            std::vector<asmjit::Label> m_block_labels{};
            JitManager* m_manager{ nullptr };
            std::unique_ptr<IRManager> m_ir{};
            asmjit::CodeHolder m_code{};
            LinearRegisterAllocator m_register_allocator{};
            std::unordered_map<uint32_t, uint32_t> m_spill_mapping{};
            std::vector<uint32_t> m_spill_free_offsets{};
            uint32_t m_last_spill_offset{ 0 };
            std::vector<asmjit::BaseNode*>
                m_restore_locations{}; // This stores a list of nodes which need to have a register restore bound

            constexpr static auto StackPointer = asmjit::x86::rsp;
            constexpr static auto CoreStatePointer = asmjit::x86::rbp;
        };

    private:
        std::unordered_map<uint16_t, JitBlock> m_blocks{};
        CoreState* m_core_state{ nullptr };
        asmjit::JitRuntime m_rt{};
        JitBlock* m_execution_block{ nullptr };
    };
} // namespace jip
