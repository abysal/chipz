#include "chip_core.hpp"
#include "util/template.hpp"

namespace cip {
    const FnBlock& ChipCore::get_group_f_funcs() noexcept {
        constexpr static auto jump_table = [] consteval {
            FnBlock j_table{};

            expand_sequence<uint8_t, std::make_integer_sequence<uint8_t, RegisterCount>>::call<[]<uint8_t reg
                >(auto& jmp_table) {
                    jmp_table[static_cast<uint16_t>(reg) << 8 | 0x65] = +[] {
                        core->t_read_bytes_v0_to_vx<reg>();
                    };
                    jmp_table[static_cast<uint16_t>(reg) << 8 | 0x55] = +[] {
                        core->t_write_bytes_v0_to_vx<reg>();
                    };

                    jmp_table[static_cast<uint16_t>(reg) << 8 | 0x33] = +[] {
                        core->t_write_bcd<reg>();
                    };

                    jmp_table[static_cast<uint16_t>(reg) << 8 | 0x1E] = +[] {
                        core->t_increment_i<reg>();
                    };

                    jmp_table[static_cast<uint16_t>(reg) << 8 | 0x29] = +[] {
                        core->t_set_i_font<reg>();
                    };

                    jmp_table[static_cast<uint16_t>(reg) << 8 | 0x18] = +[] {

                    };

                    jmp_table[static_cast<uint16_t>(reg) << 8 | 0x15] = +[] {
                        core->t_read_timer<reg>();
                    };

                    jmp_table[static_cast<uint16_t>(reg) << 8 | 0x7] = +[] {
                        core->t_write_timer<reg>();
                    };

                }>(j_table);

            return j_table;
        }();

        return jump_table;

    }
}