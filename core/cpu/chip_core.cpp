#include "chip_core.hpp"


#include <chrono>
#include <random>
#include <print>
#include <thread>

namespace cip {
    ChipCore* core{ nullptr };

    static std::array<ChipFn, std::numeric_limits<uint16_t>::max()> function_table{};

    ChipCore::ChipCore(FrontEndManager& manager) : m_manager(manager), m_memory(0x1000) {
        this->load_font();
        this->load_fns();
        core = this;
    }

    void ChipCore::load(std::span<const uint8_t> data) {
        this->m_ip = 0x200;
        std::ranges::copy(data, this->m_memory.begin() + this->m_ip);
    }

    void ChipCore::run_normal() {
        using namespace std::chrono_literals;
        const auto instructions_per_batch = this->m_ips / this->m_target_fps;
        const auto time_per_batch = (1s) / static_cast<double>(this->m_target_fps);
        const auto time_per_batch_ns = duration_cast<std::chrono::nanoseconds>(time_per_batch);
        auto start = std::chrono::high_resolution_clock::now();
        size_t instructions_executed = 0;

        while (!this->m_manager.stop()) {
            if (this->m_timer > 0) {
                --this->m_timer;
            }

            start = std::chrono::high_resolution_clock::now();
            for (size_t i = 0; i < instructions_per_batch; ++i) {
                this->execute(this->fetch());
                ++instructions_executed;
            }

            this->m_manager.update_fb(this->m_display);

            const auto end = std::chrono::high_resolution_clock::now();
            const auto time_taken = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            if (time_per_batch_ns > time_taken) {
                std::this_thread::sleep_for(time_per_batch_ns - time_taken);
            } else {
                std::println("Lagged: {}", (time_taken - time_per_batch_ns).count());
            }
        }
    }

    void ChipCore::run() noexcept {
        using namespace std::chrono_literals;

        if (this->m_high_clock) {
            size_t instructions_per_batch = 3000000;
            size_t instructions_executed = 0;
            const auto time_per_batch = (1s) / static_cast<double>(this->m_target_fps);
            const auto time_per_batch_ns = duration_cast<std::chrono::nanoseconds>(time_per_batch).count();
            size_t batch_count = 0;
            while (!this->m_manager.stop()) {
                if (this->m_timer > 0) {
                    --this->m_timer;
                }

                if (batch_count % 60 == 0) {
                    std::println("{}", instructions_per_batch);
                }
                batch_count++;
                auto before_execution = std::chrono::high_resolution_clock::now();
                for (size_t i = 0; i < instructions_per_batch; ++i) {
                    this->execute(this->fetch());
                    ++instructions_executed;
                }

                this->m_manager.update_fb(this->m_display);
                const auto end = std::chrono::high_resolution_clock::now();
                const auto execution_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - before_execution)
                    .count();
                if (time_per_batch_ns > execution_time) {
                    instructions_per_batch = static_cast<size_t>(static_cast<float>(instructions_per_batch) * 1.001f);
                } else {
                    instructions_per_batch = static_cast<size_t>(static_cast<float>(instructions_per_batch) * 0.999f);
                }
            }
        } else {
            this->run_normal();
        }

        std::println("Core execution finished");
        this->m_manager.set_finished(true);

    }

    void ChipCore::execute(const uint16_t instruction) {
        function_table[instruction]();
    }

    void ChipCore::load_font() noexcept {
        constexpr std::array<uint8_t, 80> chip8_fontset = {
            // 0
            0xF0, 0x90, 0x90, 0x90, 0xF0,
            // 1
            0x20, 0x60, 0x20, 0x20, 0x70,
            // 2
            0xF0, 0x10, 0xF0, 0x80, 0xF0,
            // 3
            0xF0, 0x10, 0xF0, 0x10, 0xF0,
            // 4
            0x90, 0x90, 0xF0, 0x10, 0x10,
            // 5
            0xF0, 0x80, 0xF0, 0x10, 0xF0,
            // 6
            0xF0, 0x80, 0xF0, 0x90, 0xF0,
            // 7
            0xF0, 0x10, 0x20, 0x40, 0x40,
            // 8
            0xF0, 0x90, 0xF0, 0x90, 0xF0,
            // 9
            0xF0, 0x90, 0xF0, 0x10, 0xF0,
            // A
            0xF0, 0x90, 0xF0, 0x90, 0x90,
            // B
            0xE0, 0x90, 0xE0, 0x90, 0xE0,
            // C
            0xF0, 0x80, 0x80, 0x80, 0xF0,
            // D
            0xE0, 0x90, 0x90, 0x90, 0xE0,
            // E
            0xF0, 0x80, 0xF0, 0x80, 0xF0,
            // F
            0xF0, 0x80, 0xF0, 0x80, 0x80
        };

        std::ranges::copy(chip8_fontset, this->m_memory.begin());
    }


    void ChipCore::load_fn_block(const uint8_t byte_prefix, const FnBlock& block) {
        std::ranges::copy(block, function_table.begin() + (static_cast<uint16_t>(byte_prefix) << 12));
    }

    void ChipCore::load_fns() noexcept {
        this->load_fn_block(0x1, get_jump_funcs());
        this->load_fn_block(0x2, get_call_funcs());
        this->load_fn_block(0x3, get_skip_imm_eq_funcs());
        this->load_fn_block(0x4, get_skip_imm_neq_funcs());
        this->load_fn_block(0x5, get_skip_reg_eq_funcs());
        this->load_fn_block(0x6, get_set_funcs());
        this->load_fn_block(0x8, get_group8_funcs());
        this->load_fn_block(0x7, get_add_funcs());
        this->load_fn_block(0x9, get_skip_reg_neq_funcs());
        this->load_fn_block(0xA, get_set_i_funcs());
        this->load_fn_block(0xB, get_long_jump_funcs());
        this->load_fn_block(0xD, get_draw_sprite_funcs());
        this->load_fn_block(0xF, get_group_f_funcs());
        function_table[0xE0] = +[] {
            core->m_display.clear();
        };
        function_table[0xEE] = +[] {
            core->m_ip = core->m_stack.pop();
        };
    }
} // cip