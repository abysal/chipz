#pragma once
#include <cstdint>

namespace jip {
    class JitBlock {
    public:
        explicit JitBlock(void* memory) : m_jitted_code(memory) {}

        JitBlock(const JitBlock&) = default;
        JitBlock& operator=(const JitBlock&) = default;
        JitBlock(JitBlock&&) = default;
        JitBlock& operator=(JitBlock&&) = default;

        uint16_t execute() const;

    private:
        void* m_jitted_code{ nullptr };
    };
} // namespace jip