#pragma once
#include "util/types.hpp"

#include <cassert>
#include <concepts>

namespace cip::hlir {

    class Register {
    public:
        constexpr Register() = default;

        constexpr Register(const u16 bit_width, const u32 assigned_id, const bool is_cpu)
            : m_assigned_id(assigned_id), m_bit_width(bit_width), m_is_cpu(is_cpu) {}

        [[nodiscard]] constexpr u16 bit_width() const noexcept { return this->m_bit_width; }

        [[nodiscard]] constexpr u32 assigned_id() const noexcept { return this->m_assigned_id; }

        [[nodiscard]] constexpr bool is_cpu() const noexcept { return m_is_cpu; }
        [[nodiscard]] constexpr bool is_temp() const noexcept { return !this->m_is_cpu; }
        [[nodiscard]] constexpr bool is_ptr() const noexcept { return this->m_is_pointer_register; }

        constexpr void set_is_ptr(const bool val) noexcept { this->m_is_pointer_register = val; }

    protected:
        u32 m_assigned_id{};
        u16 m_bit_width{};
        bool m_is_pointer_register{};
        bool m_is_cpu{ false };
    };

    template <u16 size>
    class SizedRegister : public Register {
    public:
        SizedRegister(const u32 assigned_id, const bool is_cpu) : Register(size, assigned_id, is_cpu) {}

        static SizedRegister from_reg(const Register reg) noexcept {
            assert(reg.bit_width() == size);
            return SizedRegister(reg.assigned_id(), reg.is_cpu());
        }

        auto operator<=>(const SizedRegister&) const noexcept = default;
    };

} // namespace cip::hlir
