#include "instruction_info.hpp"

namespace jip {
    InstructionType compute_type(const uint16_t instr) {
        constexpr static auto base_op_code_mask = 0xF000;

        switch ((instr & base_op_code_mask) >> 12) {
        case 0x0:
            return InstructionType::Native;
        case 0x1:
            return InstructionType::Jump;
        case 0x2:
            return InstructionType::Call;
        case 0x3:
            return InstructionType::SkipEqRegImm;
        case 0x4:
            return InstructionType::SkipNeRegImm;
        case 0x5:
            return InstructionType::SkipEqRegReg;
        case 0x6:
            return InstructionType::LoadImm;
        case 0x7:
            return InstructionType::AddImm;
        case 0x8: {
            switch (instr & 0xF) {
            case 0x0:
                return InstructionType::MovReg;
            case 0x1:
                return InstructionType::RegOr;
            case 0x2:
                return InstructionType::RegAnd;
            case 0x3:
                return InstructionType::RegXor;
            case 0x4:
                return InstructionType::RegAddYX;
            case 0x5:
                return InstructionType::RegSubYX;
            case 0x6:
                return InstructionType::RegShrXY;
            case 0x7:
                return InstructionType::RegSubXY;
            case 0xE:
                return InstructionType::RegShlXY;
            default:
                return InstructionType::Invalid;
            }
        }
        case 0x9:
            return InstructionType::SkipNeRegReg;
        case 0xA:
            return InstructionType::LoadImmI;
        case 0xB:
            return InstructionType::LongJump;
        case 0xC:
            return InstructionType::Random;
        case 0xD:
            return InstructionType::Draw;
        case 0xE: {
            if ((instr & 0xFF) == 0x9E) {
                return InstructionType::SkipKeyDown;
            }

            if ((instr & 0xFF) == 0xA1) {
                return InstructionType::SkipKeyUp;
            }
            return InstructionType::Invalid;
        }
        case 0xF: {
            switch (instr & 0xFF) {
            case 0x7:
                return InstructionType::LoadRegDelay;
            case 0xA:
                return InstructionType::LoadDelayReg;
            case 0x15:
                return InstructionType::WaitKeyPress;
            case 0x18:
                return InstructionType::SetSoundReg;
            case 0x1E:
                return InstructionType::IAddReg;
            case 0x29:
                return InstructionType::LoadFont;
            case 0x33:
                return InstructionType::BCD;
            case 0x55:
                return InstructionType::RangeWrite;
            case 0x65:
                return InstructionType::RangeRead;
            default:
                return InstructionType::Invalid;
            }
        }
        default:
            return InstructionType::Invalid;
        }

    }

}

