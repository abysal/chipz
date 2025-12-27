#pragma once
#include "instruction_info.hpp"
#include "util/memory_stream.hpp"
#include "util/static_stack.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace jip {
    class Instruction {
    public:
        Instruction() = default;

        [[nodiscard]] uint8_t regs_used() const noexcept { return m_regs_used; }
        [[nodiscard]] uint16_t immediate() const noexcept { return m_immediate; }
        [[nodiscard]] bool changes_control_flow() const noexcept { return m_changes_control_flow; }
        [[nodiscard]] InstructionType type() const noexcept { return m_type; }
        [[nodiscard]] const auto& used_regs() const noexcept { return m_used_regs; }

        [[nodiscard]] bool is_skip_next() const noexcept;

    private:
        Instruction(
            const uint8_t used_regs, const bool changes_control_flow, const InstructionType type,
            const cip::StaticVector<uint8_t, 2, uint8_t> regs, const uint16_t immediate
        ) noexcept
            : m_regs_used(used_regs), m_immediate(immediate), m_used_regs(regs),
              m_changes_control_flow(changes_control_flow), m_type(type) {}

        friend class InstructionList;
        uint16_t m_immediate{ 0 };
        uint8_t m_regs_used{ 0 };
        bool m_changes_control_flow{ false };
        cip::StaticVector<uint8_t, 2, uint8_t> m_used_regs{};
        InstructionType m_type{};
    };

    class InstructionList {
    public:
        [[nodiscard]] auto begin() noexcept { return m_instructions.begin(); }
        [[nodiscard]] auto end() noexcept { return m_instructions.end(); }

        [[nodiscard]] auto begin() const noexcept { return m_instructions.begin(); }
        [[nodiscard]] auto end() const noexcept { return m_instructions.end(); }

        [[nodiscard]] Instruction at(const uint16_t index) const noexcept { return m_instructions.at(index); }

        void create_block(MemoryStream stream, uint16_t ip);
        const auto& jump_points() const noexcept { return m_local_jump_points; }

    private:
        std::optional<Instruction> decode_instruction(uint16_t bytes);

    private:
        std::vector<Instruction> m_instructions;
        std::vector<uint32_t> m_local_jump_points;
    };
} // namespace jip