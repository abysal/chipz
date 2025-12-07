#pragma once
#include <utility>

namespace cip {

    template <typename Seq, typename...>
    struct expand_sequence;

    template <typename int_type, int_type ... vals>
    struct expand_sequence<int_type, std::integer_sequence<int_type, vals...>> {
        template <auto callback, typename... Args>
        static constexpr void call(Args&&... args) noexcept {
            ((callback.template operator()<vals>(std::forward<Args>(args)...)), ...);
        }
    };

}