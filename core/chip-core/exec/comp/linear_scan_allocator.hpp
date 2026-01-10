#pragma once
#include "asmjit/x86.h"
#include "chip-core/cpu_info.hpp"
#include "chip-core/exec/ir/register_allocator.hpp"
#include "util/types.hpp"

#include <vector>

namespace cip::hlir {
    class HLIR;
}

namespace cip::comp {
    using RegType = asmjit::x86::Gp;

    struct RegisterSelector {
        RegType bit8{};
        RegType bit16{};
        RegType bit32{};
        RegType bit64{};

        [[nodiscard]] RegType get_n_reg(u16 bit_width) const noexcept;

        bool operator==(const RegisterSelector&) const = default;
        bool operator!=(const RegisterSelector&) const = default;
    };

    struct RegisterAllocation {
        struct RegisterInfo {
            bool is_cpu{};
            u32 vreg{};
            GPCpuRegs cpu_reg{ GPCpuRegs::INVALID };
            SpecialCPURegs special_cpu_regs{ SpecialCPURegs::INVALID };
        };

        RegType target_register{};
        bool needs_restoring{};
        bool requires_save{};
        uint16_t old_reg_size{};
        RegisterInfo loaded_info{};
        RegisterInfo save_target_info{};
    };

    class LinearScanAllocator {
    public:
        LinearScanAllocator() = default;

        void trace_accesses(const hlir::HLIR&);

        void set_free_registers(std::vector<RegisterSelector> registers) { m_free_registers = std::move(registers); }
        RegisterAllocation allocate_register(u32 vreg, u32 ip);

    private:
        [[nodiscard]] bool compute_correctness() const;
        [[nodiscard]] bool next_access_write_only(u32 vreg, u32 ip) const;
        [[nodiscard]] u32 next_access_distance(u32 vreg, u32 ip) const;

    private:
        struct AccessPointInfo {
            u32 current_ip{};
            bool read{};
            bool write{};
        };

        struct ActiveRegister : RegisterSelector {
            u32 reg_id{};
        };

        struct RegisterLivingInfo {
            std::vector<AccessPointInfo> info{};
        };

        std::vector<RegisterLivingInfo> m_register_living_info{};
        hlir::RegisterAllocator m_allocator{};

        std::vector<ActiveRegister> m_used_registers{};
        std::vector<RegisterSelector> m_free_registers{};
    };

} // namespace cip::comp
