#pragma once
#include "instr/instruction.hpp"
#include "util/types.hpp"

#include <string>
#include <vector>

namespace cip::chip {

    class Disassembly {
    public:
        constexpr Disassembly() = default;
        constexpr Disassembly(const Disassembly&) = default;
        constexpr Disassembly(Disassembly&&) noexcept = default;
        constexpr Disassembly& operator=(const Disassembly&) = default;
        constexpr Disassembly& operator=(Disassembly&&) noexcept = default;
        constexpr explicit Disassembly(const u16 start_pc) : m_start_pc(start_pc) {}

        constexpr void push_instruction(chip::Instruction instr) noexcept { this->m_instructions.emplace_back(instr); }

        constexpr auto begin() const noexcept { return this->m_instructions.begin(); }
        constexpr auto end() const noexcept { return this->m_instructions.end(); }

        constexpr u16 start_pc() const noexcept { return this->m_start_pc; }

        std::string as_string() const noexcept;

    private:
        u16 m_start_pc{};
        std::vector<chip::Instruction> m_instructions{};
    };

} // namespace cip::chip
