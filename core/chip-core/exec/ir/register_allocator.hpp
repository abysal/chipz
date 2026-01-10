#pragma once
#include "chip-core/cpu_info.hpp"
#include "hlir/register.hpp"

#include <utility>
#include <vector>

namespace cip::hlir {

    class RegisterAllocator {
    public:
        RegisterAllocator() = default;

        SizedRegister<8> new_8();
        SizedRegister<16> new_16();
        SizedRegister<32> new_32();
        SizedRegister<64> new_64();
        SizedRegister<64> new_ptr();
        SizedRegister<256> new_256();

        SizedRegister<8> cpu_reg(GPCpuRegs reg);
        SizedRegister<16> cpu_special_reg(SpecialCPURegs reg);

        [[nodiscard]] size_t allocated_registers() const noexcept { return this->m_registers.size(); }

        [[nodiscard]] bool is_cpu(u32 id) const noexcept;

        [[nodiscard]] const auto& registers() const noexcept { return this->m_registers; }

        [[nodiscard]] GPCpuRegs get_cpu_reg_for(u32) const noexcept;
        [[nodiscard]] SpecialCPURegs get_special_cpu_reg_for(u32) const noexcept;

        [[nodiscard]] Register reg_at(const u32 n) const noexcept { return this->m_registers.at(n); }

    private:
        std::vector<std::pair<GPCpuRegs, u32>> m_cpu_regs_map{};
        std::vector<std::pair<SpecialCPURegs, u32>> m_special_regs_map{};
        std::vector<Register> m_registers{};
        u32 m_next_reg_id{ 0 };
    };

} // namespace cip::hlir
