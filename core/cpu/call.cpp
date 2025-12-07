#include "chip_core.hpp"
#include "util/template.hpp"

const cip::FnBlock& cip::ChipCore::get_call_funcs() noexcept {
    constexpr static auto jump_table = [] consteval {
        FnBlock j_table{};
        expand_sequence<uint8_t, std::make_integer_sequence<uint8_t, RegisterCount>>::call<[]<uint8_t upper
            >(auto& jmp_table) {
                expand_sequence<uint16_t, std::make_integer_sequence<uint16_t, 256>>::call<[]<uint16_t lower
                    >(auto& jp_table) {
                        jp_table[static_cast<uint16_t>(upper) << 8 | lower] = +[] {
                            core->t_call<static_cast<uint16_t>(upper) << 8 | lower>();
                        };
                    }>(jmp_table);

            }>(j_table);

        return j_table;
    }();

    return jump_table;
}

