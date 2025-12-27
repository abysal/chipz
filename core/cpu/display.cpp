#include "display.hpp"

namespace cip {
    void Display::clear() noexcept {
        std::ranges::fill(this->m_pixels, 1);
    }

    void Display::draw_sprite(uint8_t x, uint8_t y, const std::span<const uint8_t> sprite_data) noexcept {
        x &= width - 1;
        y &= height - 1;

        for (size_t row = 0; row < sprite_data.size(); row++) {
            uint8_t byte = sprite_data[row];
            uint16_t dy = (y + row) & (height - 1);

            for (int col = 0; col < 8; col++) {
                uint16_t dx = (x + col);

                if ((byte & (0x80 >> col)) != 0) {
                    set_pixel(dx, dy, 1);
                }
            }
        }
    }

    void Display::set_pixel(const uint8_t x, const uint8_t y, const uint8_t value) noexcept {
        if (x >= width || y >= height) {
            return;
        }

        const auto index = y * width + x;
        this->m_pixels[index] ^= value;
    }

}