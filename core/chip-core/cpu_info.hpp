#pragma once
#include "util/types.hpp"

namespace cip {
    enum class GPCpuRegs : u8 {
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
        INVALID
    };

    enum class SpecialCPURegs : u8 {
        IndexPointer,
        ProgramCounter,
        INVALID
    };
} // namespace cip