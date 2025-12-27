#pragma once
#include "core.hpp"
#include "jit/jit_manager.hpp"

#include <cstdint>

namespace jip {
    class JpuCore : public CoreState {
    public:
        JpuCore() = default;
        ~JpuCore() = default;
        JpuCore(const JpuCore&) = delete;
        JpuCore& operator=(const JpuCore&) = delete;
        JpuCore(JpuCore&&) = delete;
        JpuCore& operator=(JpuCore&&) = delete;

        void load(std::span<const uint8_t> memory) noexcept;

    private:
        JitManager m_jit{};
    };
} // cip