#include "disassembler.hpp"

#include <stdexcept>
#include <utility>

namespace cip::chip {
    Disassembly Disassembler::create_block_disassembly() {
        Disassembly disassembly{ this->m_start_pc };
        u16 current_pc = this->m_start_pc;

        while (true) {
            const auto instr = this->try_disassemble();
            if (instr.type() == InstructionType::Invalid) {
                break;
            }
            disassembly.push_instruction(instr);

            if (!this->m_allow_control_flow_change) {
                if (instr.is_terminator()) {
                    break;
                }
            } else {
                this->m_allow_control_flow_change = false;
            }

            if (instr.is_skip()) {
                this->m_allow_control_flow_change = true;
            }

            current_pc += 2;
        }

        return disassembly;
    }

    Instruction Disassembler::try_disassemble() {
        const auto instr = this->m_memory.next_word();
        const auto type = get_instr_type(instr);

        if (type == InstructionType::Invalid) {
            return Instruction(type, 0, 0, 0);
        }
        const auto inst = Disassembler::parse_word(type, instr);
        return inst;
    }

    Instruction Disassembler::parse_word(InstructionType type, u16 word) {
        switch (type) {
        case InstructionType::Native:
        case InstructionType::Jump:
        case InstructionType::Call:
        case InstructionType::LoadImmI:
        case InstructionType::LongJump:
            return { type, 0, 0, static_cast<u16>(word & 0xFFF) };
        case InstructionType::SkipEqRegImm:
        case InstructionType::SkipNeRegImm:
        case InstructionType::LoadImm:
        case InstructionType::AddImm:
        case InstructionType::Random:
            return { type, static_cast<u8>((word >> 8) & 0xF), 0, static_cast<u16>(word & 0xFF) };
        case InstructionType::SkipEqRegReg:
        case InstructionType::SkipNeRegReg:
        case InstructionType::MovReg:
        case InstructionType::RegOr:
        case InstructionType::RegAnd:
        case InstructionType::RegXor:
        case InstructionType::RegAddYX:
        case InstructionType::RegSubXY:
        case InstructionType::RegSubYX:
        case InstructionType::RegShlXY:
        case InstructionType::RegShrXY:
            return { type, static_cast<u8>((word >> 8) & 0xF), static_cast<u8>((word & 0xF0) >> 4), 0 };
        case InstructionType::SkipKeyDown:
        case InstructionType::SkipKeyUp:
        case InstructionType::LoadDelayReg:
        case InstructionType::WaitKeyPress:
        case InstructionType::LoadRegDelay:
        case InstructionType::IAddReg:
        case InstructionType::BCD:
        case InstructionType::LoadFont:
        case InstructionType::RangeWrite:
        case InstructionType::SetSoundReg:
        case InstructionType::RangeRead:
            return { type, static_cast<u8>((word >> 8) & 0xF), 0, 0 };
        case InstructionType::Draw:
            return { type,
                     static_cast<u8>((word >> 8) & 0xF),
                     static_cast<u8>((word & 0xF0) >> 4),
                     static_cast<u16>(word & 0x0F) };
        default:
            throw std::logic_error("Somehow this happened????");
        }
    }
} // namespace cip::chip