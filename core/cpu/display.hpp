#pragma once
#include <cstdint>
#include <span>
#include <vector>

namespace cip {
    constexpr size_t width = 0x40;
    constexpr size_t height = 0x20;

    class Display {
    public:
        Display() = default;

        void clear() noexcept;

        void draw_sprite(uint8_t x, uint8_t y, std::span<const uint8_t> sprite_data) noexcept;

        [[nodiscard]] bool is_set(const uint8_t x, const uint8_t y) const noexcept {
            return this->m_pixels[y * width + x];
        }

    private:
        void set_pixel(uint8_t x, uint8_t y, uint8_t value) noexcept;

    private:
        std::array<uint8_t, width * height> m_pixels;
    };

    struct FrontEndManager {
        virtual ~FrontEndManager() = default;
        virtual void update_fb(Display) = 0;
        virtual bool stop() = 0;
        virtual void set_finished(bool) = 0;
    };

}