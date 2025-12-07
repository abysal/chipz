#pragma once
#include <array>
#include <assert.h>

namespace cip {
    template <typename T, std::size_t N>
    class StaticStack {
    public:
        StaticStack() = default;
        ~StaticStack() = default;
        StaticStack(const StaticStack&) = default;
        StaticStack& operator=(const StaticStack&) = default;
        StaticStack(StaticStack&&) = default;
        StaticStack& operator=(StaticStack&&) = default;

        void push(const T& value) noexcept {
            this->m_storage[this->get_next_index()] = value;
        }

        void push(T&& value) noexcept {
            this->m_storage[this->get_next_index()] = std::move(value);
        }

        T pop() noexcept {
            const auto idx = --this->m_size;
            return std::move(this->m_storage[idx]);
        }

    private:
        std::size_t get_next_index() {
            assert(this->m_size < N);
            return this->m_size++;
        }

    private:
        std::array<T, N> m_storage{};
        std::size_t m_size{ 0 };
    };
} // cip