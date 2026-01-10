#include "linear_scan_allocator.hpp"

#include "chip-core/exec/ir/hlir.hpp"

#include <algorithm>
#include <ranges>

namespace cip::comp {
    RegType RegisterSelector::get_n_reg(const u16 bit_width) const noexcept {
        if (bit_width == 8) {
            return this->bit8;
        }
        if (bit_width == 16) {
            return this->bit16;
        }
        if (bit_width == 32) {
            return this->bit32;
        }
        if (bit_width == 64) {
            return this->bit64;
        }
        throw std::out_of_range("");
    }

    void LinearScanAllocator::trace_accesses(const hlir::HLIR& hlir) {
        auto& allocator = hlir.allocator();
        this->m_allocator = allocator;

        this->m_register_living_info.resize(allocator.allocated_registers());
        for (const auto& [ip, instr] :
             hlir.blocks() |
                 std::ranges::views::transform([&](const auto& block) -> const auto& { return block.instructions; }) |
                 std::views::join | std::views::enumerate) {

            const auto& access = instr->modification_info();

            for (const auto info : access) {
                this->m_register_living_info[info.reg.assigned_id()].info.emplace_back(
                    AccessPointInfo{ static_cast<u32>(ip),
                                     (info.modification_type & hlir::RegisterModificationType::Read) ==
                                         hlir::RegisterModificationType::Read,
                                     (info.modification_type & hlir::RegisterModificationType::Write) ==
                                         hlir::RegisterModificationType::Write }
                );
            }
        }

        assert(this->compute_correctness());
    }

    RegisterAllocation LinearScanAllocator::allocate_register(const u32 vreg, u32 ip) {
        const auto vreg_info = this->m_allocator.reg_at(vreg);

        // Handle case of us already having it allocated
        for (const auto& reg : this->m_used_registers) {
            if (reg.reg_id == vreg) {
                return { .target_register = reg.get_n_reg(vreg_info.bit_width()),
                         .needs_restoring = false,
                         .requires_save = false };
            }
        }

        const auto write_only = this->next_access_write_only(vreg, ip);

        // Allocates simply from a pool of free registers
        if (!this->m_free_registers.empty()) {
            auto used_reg = this->m_free_registers.back();
            this->m_free_registers.pop_back();
            RegisterAllocation allocation{
                .target_register = used_reg.get_n_reg(vreg_info.bit_width()),
                .needs_restoring = !write_only,
                .requires_save = false,
                .loaded_info = { .is_cpu = vreg_info.is_cpu(),
                                .vreg = vreg,
                                .cpu_reg = this->m_allocator.get_cpu_reg_for(vreg),
                                .special_cpu_regs = this->m_allocator.get_special_cpu_reg_for(vreg) }
            };

            this->m_used_registers.emplace_back(ActiveRegister{ std::move(used_reg), static_cast<u32>(vreg) });
            return allocation;
        }

        // If we have no free registers, we check all active registers and see if any of their next accesses are a write
        // without a read. Meaning the value in them is going to be overridden anyway.

        for (auto& reg : this->m_used_registers) {
            if (this->next_access_write_only(reg.reg_id, ip)) {
                const auto old_id = reg.reg_id;
                reg.reg_id = vreg;
                return RegisterAllocation{
                    .target_register = reg.get_n_reg(vreg_info.bit_width()),
                    .needs_restoring = !write_only,
                    .requires_save = false,
                    .old_reg_size =  this->m_allocator.reg_at(old_id).bit_width(),
                    .loaded_info = { .is_cpu = vreg_info.is_cpu(),
                                        .vreg = vreg,
                                        .cpu_reg = this->m_allocator.get_cpu_reg_for(vreg),
                                        .special_cpu_regs = this->m_allocator.get_special_cpu_reg_for(vreg), }
                };
            }
        }

        // In the case where all active registers are used, and have a read next. We spill the register which is used
        // furthest in the future

        const auto [_, spilled, index] = std::ranges::max(
            this->m_used_registers | std::ranges::views::enumerate |
                std::ranges::views::transform([&](const std::pair<size_t, ActiveRegister>& info) {
                    return std::tuple{ this->next_access_distance(info.second.reg_id, ip),
                                       info.second.reg_id,
                                       info.first };
                }),
            [](const auto& a, const auto& b) { return std::get<0>(a) < std::get<0>(b); }
        );

        auto& reg = this->m_used_registers[index];
        const auto old_id = reg.reg_id;
        reg.reg_id = vreg;

        const auto spilled_info = this->m_allocator.reg_at(spilled);

        return {
            .target_register = reg.get_n_reg(vreg_info.bit_width()),
            .needs_restoring = !write_only,
            .requires_save = true,
            .old_reg_size =  this->m_allocator.reg_at(old_id).bit_width(),
            .loaded_info = { .is_cpu = vreg_info.is_cpu(),
                                .vreg = vreg,
                                .cpu_reg = this->m_allocator.get_cpu_reg_for(vreg),
                                .special_cpu_regs = this->m_allocator.get_special_cpu_reg_for(vreg),},
            .save_target_info = {
                            .is_cpu = spilled_info.is_cpu(),
                            .vreg = spilled,
                            .cpu_reg = this->m_allocator.get_cpu_reg_for(spilled),
                            .special_cpu_regs = this->m_allocator.get_special_cpu_reg_for(spilled)
                           }
        };
    }

    bool LinearScanAllocator::compute_correctness() const {
        for (const auto [index, reg] : this->m_register_living_info | std::views::enumerate) {
            if (!this->m_allocator.reg_at(index).is_cpu()) {
                if (reg.info.empty()) {
                    continue;
                }

                const auto first_access = reg.info[0];
                if (first_access.read == true) {
                    return false;
                }
            }
        }
        return true;
    }

    // TODO: Add a way to track if a CPU register is dirty, if it isn't dirty we can just return false since the reg was
    // never updated
    bool LinearScanAllocator::next_access_write_only(const u32 vreg, const u32 ip) const {
        if (this->m_allocator.reg_at(vreg).is_cpu()) {
            return false; // CPU registers get backed up at the end of the block. meaning we have to preserve their
                          // state, even if they aren't written to
        }

        const auto& info = this->m_register_living_info[vreg].info;
        for (const auto& access : info) {
            if (access.current_ip < ip) {
                continue;
            }

            return access.write && !access.read;
        }

        return true;
    }

    u32 LinearScanAllocator::next_access_distance(const u32 vreg, const u32 ip) const {
        const auto& info = this->m_register_living_info[vreg].info;
        for (const auto& access : info) {
            if (access.current_ip < ip) {
                continue;
            }

            return access.current_ip - ip;
        }

        return u32_max; // this happens if no access to this reg is found in the future, meaning we can just yeet it
    }
} // namespace cip::comp