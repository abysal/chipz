#include "chip_core.hpp"
#include "util/template.hpp"

namespace cip {
    const FnBlock& ChipCore::get_draw_sprite_funcs() noexcept {
        constexpr static auto jump_table = [] consteval {
            FnBlock j_table{};

            expand_sequence<uint8_t, std::make_integer_sequence<uint8_t, RegisterCount>>::call<[]<uint8_t vx
                >(auto& jmp_table) {
                    expand_sequence<uint8_t, std::make_integer_sequence<uint8_t, RegisterCount>>::call<[]<uint8_t vy
                        >(auto& jp_table) {
                            expand_sequence<uint8_t, std::make_integer_sequence<uint8_t, RegisterCount>>::call<[]<
                                uint8_t n
                                >(auto& jp_table) {
                                    jp_table[static_cast<uint16_t>(vx) << 8 | (vy << 4) | n] = +[] {
                                        core->t_draw_sprite<vx, vy, n>();
                                    };
                                }>(jp_table);
                        }>(jmp_table);
                }>(j_table);

            return j_table;
        }();

        return jump_table;
    }

}