#include "linear_register_allocator.hpp"

#include <algorithm>
#include <ranges>

namespace jip {

    void LinearRegisterAllocator::track(const IRManager& manager) {
        this->m_register_map = manager.reg_temps();
        this->m_registers.resize(manager.temps().size());
        for (const auto& [access_ip, instr] :
             manager.blocks() |
                 std::ranges::views::transform([](const auto& block) -> const auto& { return block.instructions(); }) |
                 std::views::join | std::views::enumerate) {

            const auto vx = instr.vx;
            const auto vy = instr.vy;

            if (vx.has_value()) {
                const auto vx_val = *vx;
                const auto access_info = IRManager::access_info(instr);
                this->add_access_point(
                    vx_val.reg,
                    access_ip,
                    (access_info & RegisterAccessInfo::VXRead) == RegisterAccessInfo::VXRead,
                    (access_info & RegisterAccessInfo::VXWrite) == RegisterAccessInfo::VXWrite
                );
            }

            if (vy.has_value()) {
                const auto vy_val = *vy;
                const auto access_info = IRManager::access_info(instr);
                this->add_access_point(
                    vy_val.reg,
                    access_ip,
                    (access_info & RegisterAccessInfo::VYRead) == RegisterAccessInfo::VYRead,
                    (access_info & RegisterAccessInfo::VYWrite) == RegisterAccessInfo::VYWrite
                );
            }

            for (const auto& [reg, access] : instr.extra_consumed_registers) {
                this->add_access_point(
                    reg.reg,
                    access_ip,
                    (access & RegisterAccessInfo::VYRead) == RegisterAccessInfo::VYRead,
                    (access & RegisterAccessInfo::VYWrite) == RegisterAccessInfo::VYWrite
                );
            }
        }
    }

    void LinearRegisterAllocator::free_if_possible(
        uint32_t ir_ip, std::unordered_map<uint32_t, uint32_t>& temp_spill_offsets,
        std::vector<uint32_t>& free_spill_offsets,
        cip::StaticVector<UsedRegInfo, GPRegCount, uint8_t>& cpu_regs_to_store
    ) noexcept {
        std::erase_if(
            this->m_used_regs,
            [&, ir_ip, this, temp = std::ref(temp_spill_offsets), free = std::ref(free_spill_offsets)](
                const UsedRegInfo& reg
            ) {
                const auto& access_info = this->m_registers[reg.reg_index];
                if (access_info.end >= ir_ip) {
                    return false;
                }

                this->m_free_regs.push_back(reg.allocated_register);
                if (this->is_cpu_reg(reg.reg_index)) {
                    cpu_regs_to_store.push(reg);
                }

                if (temp.get().contains(reg.reg_index)) {
                    const auto index = temp.get().at(reg.reg_index);
                    free.get().emplace_back(index);
                    temp.get().erase(reg.reg_index);
                }

                return true;
            }
        );
    }

    // Actual implementation logic,
    // We first check for free registers to use
    // If that fails we check for registers which next only a write access only
    //      True: We just use that register since the value of it is ignored anyway
    // False:
    // We then look at the register which has the next furthest access from the current one and free it with a request
    // to spill
    LinearRegisterAllocator::RequiredAction
    LinearRegisterAllocator::allocate(const uint32_t reg_index, const uint32_t ir_ip) noexcept {
        for (const auto& used_reg : this->m_used_regs) {
            if (used_reg.reg_index == reg_index) {
                return RequiredAction{ .actions = Actions::None, .reg_type = used_reg.allocated_register };
            }
        }

        const auto access_is_write_only = this->next_access_is_write_only(reg_index, ir_ip);
        const auto action_base = access_is_write_only ? Actions::None : Actions::Load;

        if (!this->m_free_regs.empty()) {
            const auto reg = this->m_free_regs.back();
            this->m_free_regs.pop_back();
            this->m_used_regs.emplace_back(reg_index, reg);
 this->try_add_clobbered_register(reg);

            return { .actions = action_base, .reg_type = reg };
        }

        for (auto& reg : this->m_used_regs) {
            if (this->next_access_is_write_only(reg.reg_index, ir_ip)) {
                reg.reg_index = reg_index;
                return { .actions = action_base, .reg_type = reg.allocated_register };
            }
        }

        // We are now in a case where the only option is to spill a register, this case is usually quite rare, unless we
        // are handling massive blocks with hundreds of interactions

        size_t distance = 0;
        size_t spilled_register = 0;
        for (const auto& [idx, reg] : this->m_used_regs | std::views::enumerate) {
            const auto dst = this->compute_register_distance(reg.reg_index, ir_ip);
            if (dst > distance) {
                distance = dst;
                spilled_register = idx;
            }
        }

        const auto spilt_index = this->m_used_regs[spilled_register].reg_index;
        this->m_used_regs[spilled_register].reg_index = reg_index;

        const auto action = action_base | Actions::Spill;
        return { .actions = action,
                 .spill_info = { .register_index = spilt_index },
                 .reg_type = this->m_used_regs[spilled_register].allocated_register };
    }

    void LinearRegisterAllocator::allocation_test() noexcept {
        return;
        using namespace asmjit::x86;
        this->m_free_regs = {
            rcx,
            rdx,
        };

        this->m_registers.resize(3);
        this->add_access_point(0, 0, false, true);
        this->add_access_point(1, 0, false, true);

        this->add_access_point(0, 1, true, false);
        this->add_access_point(2, 1, false, true);
        this->add_access_point(1, 2, true, false);
        this->add_access_point(2, 2, false, true);

        {
            const auto reg = this->allocate(0, 0);
            assert(reg.actions == Actions::None);
            assert(reg.reg_type == rdx);
        }

        {
            const auto reg = this->allocate(1, 0);
            assert(reg.actions == Actions::None);
            assert(reg.reg_type == rcx);
        }

        {
            const auto reg = this->allocate(0, 1);
            assert(reg.actions == Actions::None);
            assert(reg.reg_type == rdx);
        }

        {
            const auto reg = this->allocate(2, 1);
            assert(reg.actions == (Actions::None | Actions::Spill));
            assert(reg.reg_type == rcx);
            assert(reg.spill_info.register_index == 1);
        }

        std::unordered_map<uint32_t, uint32_t> temp_spill_offsets{};
        std::vector<uint32_t> free_spill_offsets{};
        cip::StaticVector<LinearRegisterAllocator::UsedRegInfo, GPRegCount, uint8_t> vals{};

        this->free_if_possible(2, temp_spill_offsets, free_spill_offsets, vals);

        {
            const auto reg = this->allocate(1, 2);
            assert(reg.actions == Actions::Load);
            assert(reg.reg_type == rdx);
        }

        {
            const auto reg = this->allocate(2, 2);
            assert(reg.actions == Actions::None);
            assert(reg.reg_type == rcx);
        }
    }

    IRReg LinearRegisterAllocator::get_ir_reg(const uint32_t reg) const {
        for (const auto& [index, alloc_reg] : this->m_register_map) {
            if (index == reg) {
                return alloc_reg;
            }
        }
        return IRReg::Invalid;
    }

    bool LinearRegisterAllocator::next_access_is_write_only(const uint32_t reg, const uint32_t ir_ip) const noexcept {
        const auto& reg_info = this->m_registers[reg];
        const auto it = std::upper_bound(
            reg_info.accesses.begin(),
            reg_info.accesses.end(),
            ir_ip,
            [](const uint32_t compare, const AccessInfo& info) { return compare <= info.index; }
        );

        if (it == reg_info.accesses.end()) {
            return !this->is_cpu_reg(reg); // If we aren't at the end of a CPU register life cycle we can dump the value
        }

        return it->access == AccessType::Write;
    }

    RegType LinearRegisterAllocator::get_reg_for_index(const uint32_t reg_index) const noexcept {
        for (const auto& [index, alloc_reg] : this->m_used_regs) {
            if (index == reg_index) {
                return alloc_reg;
            }
        }

        std::unreachable();
    }

    void LinearRegisterAllocator::add_access_point(
        const uint32_t register_index, const uint32_t relative_ip, const bool read, const bool write
    ) noexcept {
        auto& reg_info = this->m_registers[register_index];

        if (reg_info.end < relative_ip) {
            reg_info.end = relative_ip;
        }

        if (reg_info.start == std::numeric_limits<uint32_t>::max()) {
            reg_info.start = relative_ip;
        }

        if (read && write) {
            reg_info.accesses.emplace_back(AccessType::Read | AccessType::Write, relative_ip);
        } else if (write) {
            reg_info.accesses.emplace_back(AccessType::Write, relative_ip);
        } else {
            reg_info.accesses.emplace_back(AccessType::Read, relative_ip);
        }
    }

    // Computes the distance to the next access this register takes
    size_t LinearRegisterAllocator::compute_register_distance(const uint32_t reg, const uint32_t ir_ip) const noexcept {
        const auto& reg_info = this->m_registers[reg];
        const auto it = std::upper_bound(
            reg_info.accesses.begin(),
            reg_info.accesses.end(),
            ir_ip,
            [](const uint32_t compare, const AccessInfo& info) { return compare < info.index; }
        );

        if (it == reg_info.accesses.end()) {
            return 0;
        }

        const auto next_access = *it;
        return next_access.index - ir_ip;
    }

    bool LinearRegisterAllocator::is_cpu_reg(const uint32_t reg) const noexcept {
        for (const auto& [index, type] : this->m_register_map) {
            if (index == reg) {
                return type != IRReg::Invalid;
            }
        }
        return false;
    }

    void LinearRegisterAllocator::try_add_clobbered_register(const RegType& reg) noexcept {
        if (std::ranges::contains(this->m_clobbered_registers, reg)) {
            return;
        }

        if (std::ranges::contains(this->m_clobber_aware_registers, reg)) {
            this->m_clobbered_registers.emplace_back(reg);
        }
    }
} // namespace jip