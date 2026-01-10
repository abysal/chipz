#pragma once
#include <cstdint>
#include <limits>

namespace cip {
    using u8 = std::uint8_t;
    using u16 = std::uint16_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;

    using i8 = std::int8_t;
    using i16 = std::int16_t;
    using i32 = std::int32_t;
    using i64 = std::int64_t;

    using f32 = float;
    using f64 = double;

    constexpr static u8 u8_max = std::numeric_limits<u8>::max();
    constexpr static u16 u16_max = std::numeric_limits<u16>::max();
    constexpr static u32 u32_max = std::numeric_limits<u32>::max();
    constexpr static u64 u64_max = std::numeric_limits<u64>::max();
    constexpr static i8 i8_max = std::numeric_limits<i8>::max();
    constexpr static i16 i16_max = std::numeric_limits<i16>::max();
    constexpr static i32 i32_max = std::numeric_limits<i32>::max();
    constexpr static i64 i64_max = std::numeric_limits<i64>::max();

    constexpr static f32 f32_max = std::numeric_limits<f32>::max();
    constexpr static f64 f64_max = std::numeric_limits<f64>::max();

    using JITFn = u16 (*)();
} // namespace cip