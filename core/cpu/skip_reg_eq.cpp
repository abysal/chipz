#include "chip_core.hpp"
#include "util/template.hpp"

namespace cip {
    const FnBlock& ChipCore::get_skip_reg_eq_funcs() noexcept {
        constexpr static auto jump_table = [] consteval {
            std::array<void (*)(), 4096> j_table{};

            expand_sequence<uint8_t, std::make_integer_sequence<uint8_t, RegisterCount>>::call<[]<uint8_t reg
                >(auto& jmp_table) {
                    expand_sequence<uint8_t, std::make_integer_sequence<uint8_t, RegisterCount>>::call<[]<uint8_t val
                        >(auto& jp_table) {
                            jp_table[static_cast<uint16_t>(reg) << 8 | (val << 4)] = +[] {
                                core->t_skip_reg_eq<reg, val>();
                            };
                        }>(jmp_table);
                }>(j_table);

            return j_table;
        }();

        return jump_table;
    }
}