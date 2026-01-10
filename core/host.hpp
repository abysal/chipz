#pragma once

#include "chip-core/chip_core.hpp"

#include <thread>

namespace cip {
    class Host {
    public:
        Host();
        ~Host() = default;

    private:
        void run();

    private:
        std::atomic_bool m_stop{ false };
        std::atomic_bool m_core_stopped{ false };
        std::jthread m_emulation_thread{};
        std::unique_ptr<ChipCore> m_core{};
    };
} // namespace cip