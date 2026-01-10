#include "instruction.hpp"

#include <format>
#include <utility>

namespace cip::chip {
    InstructionType get_instr_type(const u16 instr) {
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

    std::string_view Instruction::get_instr_name() const noexcept {
        switch (this->type()) {
        case InstructionType::Native:
            return "Native";
        case InstructionType::Jump:
            return "Jump";
        case InstructionType::Call:
            return "Call";
        case InstructionType::SkipEqRegImm:
            return "SkipEqRegImm";
        case InstructionType::SkipNeRegImm:
            return "SkipNeRegImm";
        case InstructionType::SkipEqRegReg:
            return "SkipEqRegReg";
        case InstructionType::LoadImm:
            return "LoadImm";
        case InstructionType::AddImm:
            return "AddImm";
        case InstructionType::MovReg:
            return "MovReg";
        case InstructionType::RegOr:
            return "RegOr";
        case InstructionType::RegAnd:
            return "RegAnd";
        case InstructionType::RegXor:
            return "RegXor";
        case InstructionType::RegAddYX:
            return "RegAddYX";
        case InstructionType::RegSubYX:
            return "RegSubYX";
        case InstructionType::RegShrXY:
            return "RegShrXY";
        case InstructionType::RegSubXY:
            return "RegSubXY";
        case InstructionType::RegShlXY:
            return "RegShlXY";
        case InstructionType::SkipNeRegReg:
            return "SkipNeRegReg";
        case InstructionType::LoadImmI:
            return "LoadImmI";
        case InstructionType::LongJump:
            return "LongJump";
        case InstructionType::Random:
            return "Random";
        case InstructionType::Draw:
            return "Draw";
        case InstructionType::SkipKeyDown:
            return "SkipKeyDown";
        case InstructionType::SkipKeyUp:
            return "SkipKeyUp";
        case InstructionType::LoadRegDelay:
            return "LoadRegDelay";
        case InstructionType::WaitKeyPress:
            return "WaitKeyPress";
        case InstructionType::LoadDelayReg:
            return "LoadDelayReg";
        case InstructionType::SetSoundReg:
            return "SetSoundReg";
        case InstructionType::IAddReg:
            return "IAddReg";
        case InstructionType::LoadFont:
            return "LoadFont";
        case InstructionType::BCD:
            return "BCD";
        case InstructionType::RangeWrite:
            return "RangeWrite";
        case InstructionType::RangeRead:
            return "RangeRead";
        case InstructionType::Invalid:
            return "Invalid";
        }
        std::unreachable();
    }

    std::string Instruction::operand_string() const noexcept {
        switch (this->m_type) {
        case InstructionType::Native:
        case InstructionType::Jump:
        case InstructionType::Call:
        case InstructionType::LoadImmI:
        case InstructionType::LongJump:
            return std::format("Imm: 0x{:x}", this->m_imm);
        case InstructionType::SkipEqRegImm:
        case InstructionType::SkipNeRegImm:
        case InstructionType::LoadImm:
        case InstructionType::AddImm:
        case InstructionType::Random:
            return std::format("Vx: V{:x}, Imm: 0x{:x}", this->m_vx, this->m_imm);
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
            return std::format("Vx: V{:x}, Vy: V{:x}", this->m_vx, this->m_vy);
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
            return std::format("Vx: V{:x}", this->m_vx);
        case InstructionType::Draw:
            return std::format("Vx: V{:x}, Vy: V{:x}, Imm: 0x{:x}", this->m_vx, this->m_vy, this->m_imm);
        default:
            throw std::logic_error("Somehow this happened????");
        }
    }
} // namespace cip::chip