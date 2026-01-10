#pragma once
#include <cstdint>
#include <span>

namespace cip {
    class MemoryStream {
    public:
        MemoryStream() = default;
        MemoryStream(const MemoryStream&) = default;
        MemoryStream(MemoryStream&&) = default;
        MemoryStream& operator=(const MemoryStream&) = default;
        MemoryStream& operator=(MemoryStream&&) = default;

        explicit MemoryStream(const std::span<const uint8_t> memory) noexcept : m_data{ memory } {}

        uint8_t next_byte() noexcept { return this->m_data[this->m_current_index++]; }

        uint16_t next_word() noexcept {
            const auto high = this->m_data[this->m_current_index++];
            const auto low = this->m_data[this->m_current_index++];
            return static_cast<uint16_t>(high) << 8 | low;
        }

        [[nodiscard]] uint16_t peek_word() const noexcept {
            const auto high = this->m_data[this->m_current_index];
            const auto low = this->m_data[this->m_current_index + 1];
            return static_cast<uint16_t>(high) << 8 | low;
        }

        [[nodiscard]] bool has_next() const noexcept { return this->m_current_index + 1 < this->m_data.size(); }

    private:
        std::span<const uint8_t> m_data{};
        size_t m_current_index{ 0 };
    };
} // namespace cip