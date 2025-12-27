#pragma once
#include "jpu/core.hpp"
#include "jpu/jit/instruction_list.hpp"

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

namespace jip {
    enum class IROpcode : uint16_t {
        Add,
        Sub,
        AddImm,
        SubImm,
        MulImm,
        DivImm,
        ModImm,
        AndImm,
        LoadImmediate,
        LoadByteFromI,
        LoadReg,
        JmpZ,
        JmpNZ,
        JmpEqImm,
        JmpNeImm,
        JmpEqReg,
        JmpNeReg,
        XorDisplayMemory,
        ClearDisplayMemory,
        ShrImm,
        JmpBlock,
        JmpJit,
        FlagRegisterCheck,

        OrRegReg,
        AndRegReg,
        XorRegReg,

        ShrOne,
        ShlOne,

        SubInverse,

        ReadStackOffset,
        WriteStackOffset,
        WriteToStackWithOffset,
        JumpToStackWithOffsetAndDecrement,

        WriteToMemory,
        ReadFromMemory,

        Unknown,
    };

    enum class IRReg : uint16_t {
        V0,
        V1,
        V2,
        V3,
        V4,
        V5,
        V6,
        V7,
        V8,
        V9,
        VA,
        VB,
        VC,
        VD,
        VE,
        VF,
        IN = offsetof(CoreState, index_register),
        Invalid,
    };

    struct RegisterPointer {
        bool is_temp{ false };
        uint32_t reg{};
    };

    enum class RegisterAccessInfo : uint8_t {
        None = 0,
        VXRead = 1,
        VXWrite = 2,
        VYRead = 4,
        VYWrite = 8,
    };

    struct IRInstruction {
        IROpcode code{ IROpcode::Unknown };
        std::optional<RegisterPointer> vx{};
        std::optional<RegisterPointer> vy{};
        uint32_t immediate{};
        uint32_t immediate_2{};
        std::vector<std::pair<RegisterPointer, RegisterAccessInfo>> extra_consumed_registers{};
    };

    class IRManager {
    public:
        IRManager() = default;
        ~IRManager() = default;
        IRManager(const IRManager&) = delete;
        IRManager& operator=(const IRManager&) = delete;
        IRManager(IRManager&&) = delete;
        IRManager& operator=(IRManager&&) = delete;

        uint32_t new_temp() noexcept {
            this->m_temps.push_back(this->m_temp_id);
            return m_temp_id++;
        }

        static RegisterAccessInfo access_info(const IRInstruction& code);

        uint32_t alloc_temp_for_reg(IRReg reg) noexcept;

        void emit(Instruction instr, uint16_t current_ip);

        [[nodiscard]] const auto& blocks() const noexcept { return this->m_blocks; }

        [[nodiscard]] const auto& reg_temps() const noexcept { return this->m_register_temps; }

        [[nodiscard]] const auto& temps() const noexcept { return this->m_temps; }

        void init_jump_points(const auto& jump_points) noexcept { this->m_new_block_points = jump_points; }

        class IRBlock {
        public:
            void emit_instruction(IRInstruction instr) noexcept { this->m_instructions.emplace_back(instr); }

            [[nodiscard]] const auto& instructions() const noexcept { return this->m_instructions; }

            explicit IRBlock(const uint16_t block_id) noexcept : m_block_id(block_id) {}

            [[nodiscard]] uint16_t block_id() const noexcept { return this->m_block_id; }

        private:
            std::vector<IRInstruction> m_instructions{};
            uint16_t m_block_id{};
        };

    private:
        void emit_load_imm(Instruction instr);
        void emit_index_imm(Instruction instr);
        void emit_add_imm(Instruction instr);
        void emit_dxyn(Instruction instr);
        void emit_dxyn_8_bit(Instruction instr);
        void emit_jump(Instruction instr);
        void emit_self_jump(Instruction instr);
        void emit_jit_jump(Instruction instr);
        void emit_skip_reg_eq_imm(Instruction instr);
        void emit_skip_reg_ne_imm(Instruction instr);
        void emit_skip_reg_eq_reg(Instruction instr);
        void emit_skip_reg_ne_reg(Instruction instr);
        void emit_call(Instruction instr, uint16_t current_ip);
        void emit_return(Instruction instr);
        void emit_mov_reg(Instruction instr);
        void emit_reg_or(Instruction instr);
        void emit_reg_and(Instruction instr);
        void emit_reg_xor(Instruction instr);
        void emit_reg_add_xy(Instruction instr);
        void emit_reg_sub_xy(Instruction instr);
        void emit_reg_sub_yx(Instruction instr);
        void emit_reg_shr_xy(Instruction instr);
        void emit_reg_shl_xy(Instruction instr);
        void emit_range_read(Instruction instr);
        void emit_range_write(Instruction instr);
        void emit_bcd(Instruction instr);

        class BlockHandle {
        public:
            BlockHandle(const BlockHandle&) = default;
            BlockHandle(BlockHandle&&) = default;
            BlockHandle& operator=(const BlockHandle&) = default;
            BlockHandle& operator=(BlockHandle&&) = default;

            [[nodiscard]] uint32_t index() const noexcept { return m_index; }

            void use_block() const noexcept { this->m_owner->m_active_handle = *this; }

        private:
            BlockHandle(IRManager* owner, const uint16_t m_block) : m_index(m_block), m_owner(owner) {}

            BlockHandle() : m_owner(nullptr) {}

            friend class IRManager;
            friend class IRBlock;
            uint16_t m_index{};
            IRManager* m_owner;
        };

        BlockHandle new_block() noexcept;

        void emit_instruction(const IRInstruction& instr) noexcept;

    private:
        uint32_t m_temp_id{ 0 };
        uint32_t m_label_id{ 0 };
        std::vector<IRBlock> m_blocks{};
        BlockHandle m_active_handle{};
        std::vector<std::pair<uint32_t, IRReg>> m_register_temps{};
        std::vector<uint32_t> m_temps{};

        std::vector<uint32_t> m_new_block_points{};
        std::unordered_map<uint32_t, uint32_t> m_block_point_to_block_index{};

        uint32_t m_block_switch_counter{ 0 };
        BlockHandle m_handle_to_switch{};
    };
} // namespace jip
