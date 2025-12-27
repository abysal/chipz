#pragma once
#include <concepts>

namespace jip {
    template <std::integral T>
    class Register {
    public:
        constexpr Register() noexcept = default;
        constexpr Register(const Register&) noexcept = default;
        constexpr Register(Register&&) noexcept = default;
        constexpr Register& operator=(const Register&) noexcept = default;
        constexpr Register& operator=(Register&&) noexcept = default;
        ~Register() noexcept = default;

        void set(T value) noexcept {
            this->m_value = value;
        }

        T value() const noexcept {
            return this->m_value;
        }

        T m_value{};
    };
} // jip