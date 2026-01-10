#include "division.hpp"

#include <array>
#include <bit>

namespace cip {

    constexpr Div8Info compute_magic_for_divisor_impl(uint8_t divisor) noexcept {

        if (divisor == 1) {
            return Div8Info{
                .mul = 1,
                .shift = 0,
            };
        }

        if ((divisor & divisor - 1) == 0) {
            uint8_t shift_count = 0;
            while ((1u << shift_count) != divisor)
                ++shift_count;
            return { .mul = 1, .shift = shift_count };
        }

        for (uint8_t s = 8;; ++s) {
            const uint32_t shift_val = 1u << s;
            const uint32_t magic = (shift_val + divisor - 1) / divisor;

            bool valid = true;
            for (uint16_t numerator = 0; numerator <= 0xFF; ++numerator) {
                const uint8_t expected = numerator / divisor;
                const auto guessed = static_cast<uint8_t>((static_cast<uint32_t>(numerator) * magic) >> s);

                if (expected != guessed) {
                    valid = false;
                    break;
                }
            }

            if (valid) {
                return Div8Info{ .mul = static_cast<uint16_t>(magic), .shift = s };
            }
        }
    }

    static_assert([] consteval {
        constexpr static auto numerators = std::array{ 21, 30, 50, 41, 83, 201, 108 };
        constexpr static auto divisors = std::array{ 1, 2, 3, 8, 9, 7, 10, 11, 31 };

        for (uint16_t numerator : numerators) {
            for (uint16_t divisor : divisors) {
                const auto magic = compute_magic_for_divisor_impl(divisor);

                const auto expected = static_cast<uint16_t>(numerator / divisor);
                const auto our_result =
                    (static_cast<uint32_t>(numerator) * magic.mul) >> static_cast<uint32_t>(magic.shift);

                if (expected != our_result) {
                    return false;
                }
            }
        }
        return true;
    }() == true);

    constexpr auto compute_lut_8() {
        std::array<Div8Info, 256> magic_array{};

        for (uint16_t divisor = 1; divisor <= 0xFF; ++divisor) {
            const auto magic = compute_magic_for_divisor_impl(divisor);
            magic_array[divisor] = magic;
        }
        return magic_array;
    }

    static auto div8_lut = compute_lut_8();

    Div8Info compute_magic_for_d(const uint8_t divisor) noexcept { return div8_lut[divisor]; }

} // namespace cip
