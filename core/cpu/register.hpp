#pragma once
#include <cstdint>

namespace cip
{
    class Register
    {
    public:
        constexpr Register() noexcept = default;

        constexpr bool add(const uint8_t value) noexcept {
            const auto our_value = this->value();
            const uint16_t val = static_cast<uint16_t>(our_value) + static_cast<uint16_t>(value);
            this->set(val);
            return (val & 0xFF00) > 0;
        }

        constexpr bool sub(const uint8_t v) noexcept {
            const auto a = value();
            const auto res = a - v;
            this->set(res);
            return a < v;
        }

        constexpr void set(const uint8_t value) noexcept {
            this->m_value = value;
        }

        [[nodiscard]] constexpr uint8_t value() const noexcept {
            return this->m_value;
        }
    private:
        uint8_t m_value{0x0};
    };
} // crip