#pragma once
#include <cstdint>

namespace jip {
    enum class InstructionType : uint16_t {
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

    InstructionType compute_type(uint16_t instr);
} // namespace jip