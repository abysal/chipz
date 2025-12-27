#pragma once
#include <type_traits>

template <typename T>
    requires std::is_enum_v<T>
constexpr T operator |(T lhs, T rhs) noexcept {
    return static_cast<T>(std::underlying_type_t<T>(lhs) | std::underlying_type_t<T>(rhs));
}

template <typename T>
    requires std::is_enum_v<T>
constexpr T operator&(T lhs, T rhs) noexcept {
    return static_cast<T>(std::underlying_type_t<T>(lhs) & std::underlying_type_t<T>(rhs));
}