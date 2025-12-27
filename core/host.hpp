#pragma once
#include "cpu/chip_core.hpp"
#include "jpu/jpu_core.hpp"

#include <thread>

namespace cip {
    class Host final : public FrontEndManager {
    public:
        Host();
        ~Host() override = default;

        bool stop() override {
            return this->m_stop;
        }

        void update_fb(Display dis) override {
            this->m_display = std::move(dis);
        }

        void set_finished(const bool val) override {
            this->m_core_stopped = val;
        }

    private:
        void run();

    public:

    private:
        Display m_display{};
        std::atomic_bool m_stop{ false };
        std::atomic_bool m_core_stopped{ false };
        std::jthread m_emulation_thread{};
        std::unique_ptr<ChipCore> m_core{};
        std::unique_ptr<jip::JpuCore> m_jit_core{};
    };
} // cip