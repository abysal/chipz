#pragma once
#include "asmjit/x86.h"
#include "ir/ir_manager.hpp"
#include "util/enum.hpp"

#include <cstdint>
#include <limits>
#include <unordered_map>
#include <vector>

namespace jip {

    using RegType = asmjit::x86::Gp;

    enum class AccessType : uint8_t {
        Read = 1,
        Write = 2,
    };

    class LinearRegisterAllocator {
    public:
        struct UsedRegInfo {
            uint32_t reg_index{};
            RegType allocated_register{};
        };

        LinearRegisterAllocator() = default;
        ~LinearRegisterAllocator() = default;
        LinearRegisterAllocator(const LinearRegisterAllocator&) = delete;
        LinearRegisterAllocator(LinearRegisterAllocator&&) = default;
        LinearRegisterAllocator& operator=(const LinearRegisterAllocator&) = delete;
        LinearRegisterAllocator& operator=(LinearRegisterAllocator&&) = default;

        void track(const IRManager& manager);

        void initialize_free_regs(std::vector<RegType> regs) noexcept { this->m_free_regs = std::move(regs); }
        void initialize_clobber_aware_registers(std::vector<RegType> regs) noexcept {
            this->m_clobber_aware_registers = std::move(regs);
        }

        void add_clobbered(const RegType& reg) noexcept { this->try_add_clobbered_register(reg); }

        enum class Actions {
            Spill = 1,
            Load = 2,
            None = 4,
        };

        struct SpillInfo {
            uint32_t register_index;
        };

        struct RequiredAction {
            Actions actions{};
            SpillInfo spill_info{};
            RegType reg_type{};
        };

        void free_if_possible(
            uint32_t ir_ip, std::unordered_map<uint32_t, uint32_t>& temp_spill_offsets,
            std::vector<uint32_t>& free_spill_offsets,
            cip::StaticVector<UsedRegInfo, GPRegCount, uint8_t>& cpu_regs_to_store
        ) noexcept;

        RequiredAction allocate(uint32_t reg_index, uint32_t ir_ip) noexcept;

        void allocation_test() noexcept;

        [[nodiscard]] IRReg get_ir_reg(uint32_t reg) const;

        [[nodiscard]] bool next_access_is_write_only(uint32_t reg, uint32_t ir_ip) const noexcept;

        [[nodiscard]] const auto& allocated_regs() const noexcept { return this->m_used_regs; }

        [[nodiscard]] const auto& clobbered_regs() const noexcept { return this->m_clobbered_registers; }

        [[nodiscard]] RegType get_reg_for_index(uint32_t reg_index) const noexcept;

    private:
        void add_access_point(uint32_t register_index, uint32_t relative_ip, bool read, bool write) noexcept;

        [[nodiscard]] size_t compute_register_distance(uint32_t reg, uint32_t ir_ip) const noexcept;

        [[nodiscard]] bool is_cpu_reg(uint32_t reg) const noexcept;

        void try_add_clobbered_register(const RegType& reg) noexcept;

    private:
        struct AccessInfo {
            AccessType access{};
            uint32_t index{};
        };

        struct RegisterLiveRange {
            uint32_t start{ std::numeric_limits<uint32_t>::max() };
            uint32_t end{ 0 };
            std::vector<AccessInfo> accesses{};
            uint32_t last_ip_sampled{};
        };

        std::vector<std::pair<uint32_t, IRReg>> m_register_map{};

        std::vector<RegType> m_clobber_aware_registers{};
        std::vector<RegType> m_clobbered_registers{};
        std::vector<RegisterLiveRange> m_registers{};

        std::vector<RegType> m_free_regs{};
        std::vector<UsedRegInfo> m_used_regs{};
    };

} // namespace jip