#pragma once
#include "disassembly.hpp"
#include "util/memory_stream.hpp"
#include "util/types.hpp"

namespace cip::chip {

    class Disassembler {
    public:
        Disassembler() = default;
        Disassembler(const MemoryStream& mem_stream, const u16 start_pc) : m_memory(mem_stream), m_start_pc(start_pc) {}
        Disassembler(const Disassembler&) = delete;
        Disassembler& operator=(const Disassembler&) = delete;
        Disassembler(Disassembler&&) = default;
        Disassembler& operator=(Disassembler&&) = default;

        Disassembly create_block_disassembly();

    private:
        Instruction try_disassemble();
        static Instruction parse_word(InstructionType type, u16 word);

    private:
        MemoryStream m_memory{};
        u16 m_start_pc{};
        bool m_allow_control_flow_change{
            false
        }; // This is used as a flag to track when we encounter a skip instruction, since they are allowed to contain a
           // control flow altering instruction in the skipped component
    };

} // namespace cip::chip
