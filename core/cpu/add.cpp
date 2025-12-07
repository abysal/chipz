#include "chip_core.hpp"
#include "util/template.hpp"

namespace cip {

    const FnBlock& ChipCore::get_add_funcs() noexcept {
        constexpr static auto jump_table = [] consteval {
            FnBlock j_table{};

            expand_sequence<uint8_t, std::make_integer_sequence<uint8_t, RegisterCount>>::call<[]<uint8_t reg
                >(auto& jmp_table) {
                    expand_sequence<uint16_t, std::make_integer_sequence<uint16_t, 256>>::call<[]<uint16_t val
                        >(auto& jp_table) {
                            jp_table[static_cast<uint16_t>(reg) << 8 | (val & 0xFF)] = +[] {
                                core->t_add<reg, val>();
                            };
                        }>(jmp_table);
                }>(j_table);

            return j_table;
        }();

        return jump_table;
    }

}