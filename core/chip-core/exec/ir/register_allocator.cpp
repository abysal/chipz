#include "register_allocator.hpp"

namespace cip::hlir {
    bool RegisterAllocator::is_cpu(const u32 id) const noexcept { return this->m_registers[id].is_cpu(); }

    GPCpuRegs RegisterAllocator::get_cpu_reg_for(const u32 cpu_reg) const noexcept {
        for (const auto& [type, reg] : this->m_cpu_regs_map) {
            if (reg == cpu_reg) {
                return type;
            }
        }

        return GPCpuRegs::INVALID;
    }

    SpecialCPURegs RegisterAllocator::get_special_cpu_reg_for(const u32 cpu_reg) const noexcept {
        for (const auto& [type, reg] : this->m_special_regs_map) {
            if (reg == cpu_reg) {
                return type;
            }
        }
        return SpecialCPURegs::INVALID;
    }

    SizedRegister<8> RegisterAllocator::new_8() {
        const SizedRegister<8> reg{ this->m_next_reg_id++, false };
        this->m_registers.push_back(reg);
        return reg;
    }

    SizedRegister<16> RegisterAllocator::new_16() {
        const SizedRegister<16> reg{ this->m_next_reg_id++, false };
        this->m_registers.push_back(reg);
        return reg;
    }

    SizedRegister<32> RegisterAllocator::new_32() {
        const SizedRegister<32> reg{ this->m_next_reg_id++, false };
        this->m_registers.push_back(reg);
        return reg;
    }

    SizedRegister<64> RegisterAllocator::new_64() {
        const SizedRegister<64> reg{ this->m_next_reg_id++, false };
        this->m_registers.push_back(reg);
        return reg;
    }

    SizedRegister<64> RegisterAllocator::new_ptr() {
        auto reg = this->new_64();
        reg.set_is_ptr(true);
        return reg;
    }

    SizedRegister<256> RegisterAllocator::new_256() {
        const SizedRegister<256> reg{ this->m_next_reg_id++, false };
        this->m_registers.push_back(reg);
        return reg;
    }

    SizedRegister<8> RegisterAllocator::cpu_reg(GPCpuRegs reg) {
        assert(reg != GPCpuRegs::INVALID);
        const auto val =
            std::ranges::find_if(this->m_cpu_regs_map, [&](const auto& mapping) { return mapping.first == reg; });

        if (val != this->m_cpu_regs_map.end()) {
            return SizedRegister<8>::from_reg(this->m_registers[val->second]);
        }
        const auto index = this->m_next_reg_id++;

        const SizedRegister<8> alloc_reg{ index, true };
        this->m_registers.push_back(alloc_reg);
        this->m_cpu_regs_map.emplace_back(reg, index);
        return alloc_reg;
    }

    SizedRegister<16> RegisterAllocator::cpu_special_reg(SpecialCPURegs reg) {
        const auto val =
            std::ranges::find_if(this->m_special_regs_map, [&](const auto& mapping) { return mapping.first == reg; });

        if (val != this->m_special_regs_map.end()) {
            return SizedRegister<16>::from_reg(this->m_registers[val->second]);
        }

        const auto index = this->m_next_reg_id++;
        const SizedRegister<16> alloc_reg{ index, true };
        this->m_registers.push_back(alloc_reg);
        this->m_special_regs_map.emplace_back(reg, index);
        return alloc_reg;
    }
} // namespace cip::hlir