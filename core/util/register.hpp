#pragma once
#include <concepts>

namespace cip {

    template <std::integral T, T UninitializedValue = 0>
    class Register {
    public:
        constexpr Register() noexcept = default;
        constexpr explicit(false) Register(T val) noexcept : m_value(val) {}
        constexpr T value() const noexcept { return m_value; }

        constexpr explicit(false) operator T() const noexcept { return m_value; }
        constexpr T operator*() const noexcept { return m_value; }

    private:
        T m_value{ UninitializedValue };
    };

} // namespace cip