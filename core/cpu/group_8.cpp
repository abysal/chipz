#include "chip_core.hpp"
#include "util/template.hpp"

const cip::FnBlock& cip::ChipCore::get_group8_funcs() noexcept {

    constexpr static auto jump_table = [] consteval {
        FnBlock j_table{};
        expand_sequence<uint8_t, std::make_integer_sequence<uint8_t, RegisterCount>>::call<[]<uint8_t target
            >(auto& jmp_table) {
                expand_sequence<uint8_t, std::make_integer_sequence<uint8_t, RegisterCount>>::call<[]<uint8_t source
                    >(auto& jp_table) {
                        jp_table[static_cast<uint16_t>(target) << 8 | (source << 4)] = +[] {
                            core->t_mov<target, source>();
                        };
                    }>(jmp_table);

                expand_sequence<uint8_t, std::make_integer_sequence<uint8_t, RegisterCount>>::call<[]<uint8_t source
                    >(auto& jp_table) {
                        jp_table[static_cast<uint16_t>(target) << 8 | (source << 4) | 1] = +[] {
                            core->t_or<target, source>();
                        };
                    }>(jmp_table);

                expand_sequence<uint8_t, std::make_integer_sequence<uint8_t, RegisterCount>>::call<[]<uint8_t source
                    >(auto& jp_table) {
                        jp_table[static_cast<uint16_t>(target) << 8 | (source << 4) | 2] = +[] {
                            core->t_and<target, source>();
                        };
                    }>(jmp_table);

                expand_sequence<uint8_t, std::make_integer_sequence<uint8_t, RegisterCount>>::call<[]<uint8_t source
                    >(auto& jp_table) {
                        jp_table[static_cast<uint16_t>(target) << 8 | (source << 4) | 3] = +[] {
                            core->t_xor<target, source>();
                        };
                    }>(jmp_table);

                expand_sequence<uint8_t, std::make_integer_sequence<uint8_t, RegisterCount>>::call<[]<uint8_t source
                    >(auto& jp_table) {
                        jp_table[static_cast<uint16_t>(target) << 8 | (source << 4) | 4] = +[] {
                            core->t_add_reg<target, source>();
                        };
                    }>(jmp_table);

                expand_sequence<uint8_t, std::make_integer_sequence<uint8_t, RegisterCount>>::call<[]<uint8_t source
                    >(auto& jp_table) {
                        jp_table[static_cast<uint16_t>(target) << 8 | (source << 4) | 5] = +[] {
                            core->t_sub_reg<target, source>();
                        };
                    }>(jmp_table);

                expand_sequence<uint8_t, std::make_integer_sequence<uint8_t, RegisterCount>>::call<[]<uint8_t source
                    >(auto& jp_table) {
                        jp_table[static_cast<uint16_t>(target) << 8 | (source << 4) | 6] = +[] {
                            core->t_shift_right<target, source>();
                        };
                    }>(jmp_table);

                expand_sequence<uint8_t, std::make_integer_sequence<uint8_t, RegisterCount>>::call<[]<uint8_t source
                    >(auto& jp_table) {
                        jp_table[static_cast<uint16_t>(target) << 8 | (source << 4) | 7] = +[] {
                            core->t_sub_reg_inverse<target, source>();
                        };
                    }>(jmp_table);

                expand_sequence<uint8_t, std::make_integer_sequence<uint8_t, RegisterCount>>::call<[]<uint8_t source
                    >(auto& jp_table) {
                        jp_table[static_cast<uint16_t>(target) << 8 | (source << 4) | 0xE] = +[] {
                            core->t_shift_left<target, source>();
                        };
                    }>(jmp_table);

            }>(j_table);

        return j_table;
    }();

    return jump_table;
}
