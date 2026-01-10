#pragma once
#include "constants.hpp"
#include "types.hpp"

#include <array>

namespace cip {

    class Display {
    public:
        Display() = default;
        Display(const Display&) = delete;
        Display& operator=(const Display&) = delete;
        Display(Display&&) = delete;
        Display& operator=(Display&&) = delete;

    private:
        std::array<u8, width * height> m_display_state{};
    };

} // namespace cip
