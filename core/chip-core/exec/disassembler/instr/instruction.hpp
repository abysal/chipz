#pragma once
#include "chip-core/cpu_info.hpp"
#include "util/types.hpp"

#include <string_view>

namespace cip::chip {

    enum class InstructionType : u16 {
        Native = 0x0,
        Jump = 0x1000,
        Call = 0x2000,
        SkipEqRegImm = 0x3000,
        SkipNeRegImm = 0x4000,
        SkipEqRegReg = 0x5000,
        LoadImm = 0x6000,
        AddImm = 0x7000,
        MovReg = 0x8000,
        RegOr = 0x8001,
        RegAnd = 0x8002,
        RegXor = 0x8003,
        RegAddYX = 0x8004,
        RegSubYX = 0x8005,
        RegShrXY = 0x8006,
        RegSubXY = 0x8007,
        RegShlXY = 0x800E,
        SkipNeRegReg = 0x9000,
        LoadImmI = 0xA000,
        LongJump = 0xB000,
        Random = 0xC000,
        Draw = 0xD000,
        SkipKeyDown = 0xE09E,
        SkipKeyUp = 0xE0A1,
        LoadRegDelay = 0xF007,
        WaitKeyPress = 0xF00A,
        LoadDelayReg = 0xF015,
        SetSoundReg = 0xF018,
        IAddReg = 0xF01E,
        LoadFont = 0xF029,
        BCD = 0xF033,
        RangeWrite = 0xF055,
        RangeRead = 0xF065,
        Invalid = 0xFFFF
    };

    InstructionType get_instr_type(u16 instr);

    class Instruction {
    public:
        constexpr Instruction() = default;
        constexpr Instruction(const InstructionType type, const u8 vx, const u8 vy, const u16 imm)
            : m_type(type), m_vx(vx), m_vy(vy), m_imm(imm) {}

        constexpr InstructionType type() const { return m_type; }
        constexpr u8 vx() const { return m_vx; }
        constexpr GPCpuRegs vx_reg() const noexcept { return static_cast<GPCpuRegs>(m_vx); }
        constexpr u8 vy() const { return m_vy; }
        constexpr GPCpuRegs vy_reg() const noexcept { return static_cast<GPCpuRegs>(m_vy); }
        constexpr u16 imm() const { return m_imm; }

        template <std::integral T>
        constexpr T imm_as() const {
            return static_cast<T>(m_imm);
        }

        std::string_view get_instr_name() const noexcept;
        std::string operand_string() const noexcept;

        constexpr bool is_skip() const noexcept {
            switch (this->m_type) {
            case InstructionType::SkipEqRegImm:
            case InstructionType::SkipNeRegImm:
            case InstructionType::SkipEqRegReg:
            case InstructionType::SkipNeRegReg:
            case InstructionType::SkipKeyDown:
            case InstructionType::SkipKeyUp:
                return true;
            default:
                return false;
            }
        }

        constexpr bool is_terminator() const noexcept {
            switch (this->m_type) {
            case InstructionType::Jump:
            case InstructionType::Call:
            case InstructionType::LongJump:
                return true;
            case InstructionType::Native:
                return this->imm() == 0xEE;
            default:
                return false;
            }
        }

    private:
        InstructionType m_type{ InstructionType::Invalid };
        u8 m_vx{};
        u8 m_vy{};
        u16 m_imm{};
    };
} // namespace cip::chip
