#pragma once
#include <array>
#include <cassert>
#include <cstdint>

namespace cip {
    template <typename T, std::size_t N, typename SizeType = std::size_t>
    class StaticVector {
    public:
        StaticVector() = default;
        ~StaticVector() = default;
        StaticVector(const StaticVector&) = default;
        StaticVector& operator=(const StaticVector&) = default;
        StaticVector(StaticVector&&) = default;
        StaticVector& operator=(StaticVector&&) = default;

        StaticVector(std::initializer_list<T> list) {
            std::copy(list.begin(), list.end(), m_storage.begin());
            this->m_size = list.size();
        }

        void push(const T& value) noexcept { this->m_storage[this->get_next_index()] = value; }

        void push(T&& value) noexcept { this->m_storage[this->get_next_index()] = std::move(value); }

        T pop() noexcept {
            const auto idx = --this->m_size;
            return std::move(this->m_storage[idx]);
        }

        T& operator[](SizeType idx) noexcept {
            assert(idx < this->m_size);
            return this->m_storage[idx];
        }

        const T& operator[](SizeType idx) const noexcept {
            assert(idx < this->m_size);
            return this->m_storage[idx];
        }

        [[nodiscard]] bool empty() const noexcept { return this->m_size == 0; }

        void clear() noexcept { this->m_size = 0; }
        auto begin() noexcept { return this->m_storage.begin(); }

        auto end() noexcept { return this->m_storage.begin() + this->m_size; }

        constexpr static size_t offset_of_size() noexcept { return offsetof(StaticVector, m_size); }

        constexpr static size_t offset_of_storage() noexcept { return offsetof(StaticVector, m_storage); }

    private:
        SizeType get_next_index() {
            assert(this->m_size < N);
            return this->m_size++;
        }

    private:
        std::array<T, N> m_storage{};
        SizeType m_size{ 0 };
    };
} // namespace cip