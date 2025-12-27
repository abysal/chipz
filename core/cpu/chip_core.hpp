#pragma once
#include "display.hpp"
#include "register.hpp"
#include "util/static_stack.hpp"

#include <array>
#include <limits>
#include <stack>
#include <vector>

namespace cip {
    class ChipCore;

    extern ChipCore* core;

    constexpr static uint8_t RegisterCount = 16;
    using ChipFn = void(*)();
    using FnBlock = std::array<ChipFn, 4096>;

    class ChipCore {
    public:
        explicit ChipCore(FrontEndManager&);
        ~ChipCore() = default;
        ChipCore(const ChipCore&) = delete;
        ChipCore& operator=(const ChipCore&) = delete;
        ChipCore(ChipCore&&) noexcept = default;
        ChipCore& operator=(ChipCore&&) noexcept = delete;

        void load(std::span<const uint8_t> data);
        void run_normal();

        void run() noexcept;

    private:
        constexpr Register& reg(const uint8_t val) noexcept {
            return this->m_registers[val];
        }

        void execute(uint16_t instruction);
        void load_font() noexcept;

        void load_fns() noexcept;
        void load_fn_block(uint8_t byte_prefix, const FnBlock& block);

        static const FnBlock& get_add_funcs() noexcept;
        static const FnBlock& get_set_funcs() noexcept;
        static const FnBlock& get_group8_funcs() noexcept;
        static const FnBlock& get_skip_imm_eq_funcs() noexcept;
        static const FnBlock& get_skip_imm_neq_funcs() noexcept;
        static const FnBlock& get_skip_reg_eq_funcs() noexcept;
        static const FnBlock& get_skip_reg_neq_funcs() noexcept;
        static const FnBlock& get_group_f_funcs() noexcept;
        static const FnBlock& get_jump_funcs() noexcept;
        static const FnBlock& get_call_funcs() noexcept;
        static const FnBlock& get_set_i_funcs() noexcept;
        static const FnBlock& get_long_jump_funcs() noexcept;
        static const FnBlock& get_draw_sprite_funcs() noexcept;

        template <uint8_t Reg, uint8_t Val>
        constexpr void t_add() noexcept {
            auto& reg = this->reg(Reg);
                reg.add(Val);
    }

        template <uint8_t Target, uint8_t Source>
        constexpr void t_add_reg() noexcept {
            auto& reg = this->reg(Target);
            const auto& source = this->reg(Source);
            auto& vf = this->reg(0xF);
            vf.set(reg.add(source.value()));
        }

        template <uint8_t Target, uint8_t Source>
        constexpr void t_sub_reg() noexcept {
            auto& reg = this->reg(Target);
            const auto& source = this->reg(Source);
            auto& vf = this->reg(0xF);
            const auto overflow = reg.sub(source.value());
            vf.set(!overflow);
        }

        template <uint8_t vx, uint8_t vy>
        constexpr void t_sub_reg_inverse() noexcept {
            auto& reg = this->reg(vx);
            const auto& source = this->reg(vy);
            auto& vf = this->reg(0xF);
            const auto reg_value = reg.value();
            const auto source_value = source.value();
            reg.set(source_value - reg_value);
            vf.set(reg_value <= source_value);
        }

        template <uint8_t Reg, uint8_t Val>
        constexpr void t_set() noexcept {
            auto& reg = this->reg(Reg);
            reg.set(Val);
        }

        template <uint8_t Target, uint8_t Source>
        constexpr void t_mov() noexcept {
            auto& t = this->reg(Target);
            const auto& s = this->reg(Source);
            t.set(s.value());
        }

        template <uint8_t Target, uint8_t Source>
        constexpr void t_or() noexcept {
            auto& t = this->reg(Target);
            const auto& s = this->reg(Source);
            auto& vf = this->reg(0xF);
            t.set(s.value() | t.value());
            vf.set(0);
        }

        template <uint8_t Target, uint8_t Source>
        constexpr void t_and() noexcept {
            auto& t = this->reg(Target);
            const auto& s = this->reg(Source);
            auto& vf = this->reg(0xF);
            t.set(s.value() & t.value());
            vf.set(0);
        }

        template <uint8_t Target, uint8_t Source>
        constexpr void t_xor() noexcept {
            auto& t = this->reg(Target);
            const auto& s = this->reg(Source);
            auto& vf = this->reg(0xF);
            t.set(s.value() ^ t.value());
            vf.set(0);
        }

        template <uint8_t Target, uint8_t Source>
        constexpr void t_shift_right() noexcept {
            auto& t = this->reg(Target);
            const auto& s = this->reg(Source);
            auto& vf = this->reg(0xF);

            const auto lsb = s.value() & 0x1;
            t.set(s.value() >> 1);
            vf.set(lsb);
        }

        template <uint8_t Target, uint8_t Source>
        constexpr void t_shift_left() noexcept {
            auto& t = this->reg(Target);
            const auto& s = this->reg(Source);
            auto& vf = this->reg(0xF);
            const auto msb = s.value() & 0x80;
            t.set(s.value() << 1);
            vf.set(msb >> 0x7);
        }

        template <uint8_t Reg, uint8_t Val>
        constexpr void t_skip_eq() noexcept {
            const auto& reg = this->reg(Reg);
            this->m_ip += sizeof(uint16_t) * static_cast<uint8_t>(reg.value() == Val);
        }

        template <uint8_t Reg, uint8_t Val>
        constexpr void t_skip_neq() noexcept {
            const auto& reg = this->reg(Reg);
            this->m_ip += sizeof(uint16_t) * static_cast<uint8_t>(reg.value() != Val);
        }

        template <uint8_t Target, uint8_t Source>
        constexpr void t_skip_reg_eq() noexcept {
            const auto& t = this->reg(Target);
            const auto& s = this->reg(Source);
            this->m_ip += sizeof(uint16_t) * static_cast<uint8_t>(t.value() == s.value());
        }

        template <uint8_t Target, uint8_t Source>
        constexpr void t_skip_reg_neq() noexcept {
            const auto& t = this->reg(Target);
            const auto& s = this->reg(Source);
            this->m_ip += sizeof(uint16_t) * static_cast<uint8_t>(t.value() != s.value());
        }

        template <uint8_t vx, uint8_t vy, uint8_t n>
        void t_draw_sprite() noexcept {
            const auto x = this->reg(vx);
            const auto y = this->reg(vy);
            const auto data = std::span{ this->m_memory }.subspan(this->m_i_register, n);
            this->m_display.draw_sprite(x.value(), y.value(), data);
        }

        template <uint8_t vx>
        void t_read_bytes_v0_to_vx() noexcept {
            auto reg_ptr = std::span{ reinterpret_cast<uint8_t*>(this->m_registers.data()), vx + 1 };
            const auto memory = std::span{ this->m_memory }.subspan(this->m_i_register, vx + 1);
            std::ranges::copy(memory, reg_ptr.begin());
            this->m_i_register += vx + 1;
        }

        template <uint8_t vx>
        void t_write_bytes_v0_to_vx() noexcept {
            const auto reg_ptr = std::span{ reinterpret_cast<uint8_t*>(this->m_registers.data()), vx + 1 };
            auto memory = std::span{ this->m_memory }.subspan(this->m_i_register, vx + 1);
            std::ranges::copy(reg_ptr, memory.begin());
            this->m_i_register += vx + 1;
        }

        template <uint8_t vx>
        void t_write_bcd() noexcept {
            const auto reg = this->reg(vx);
            auto memory = std::span{ this->m_memory };
            auto reg_val = reg.value();
            memory[this->m_i_register + 2] = reg_val % 10;
            reg_val /= 10;
            memory[this->m_i_register + 1] = reg_val % 10;
            reg_val /= 10;
            memory[this->m_i_register] = reg_val % 10;
        }

        template <uint8_t vx>
        void t_increment_i() noexcept {
            this->m_i_register += this->reg(vx).value();
        }

        template <uint8_t vx>
        void t_read_timer() noexcept {
            this->reg(vx).set(this->m_timer);
        }

        template <uint8_t vx>
        void t_write_timer() noexcept {
            this->reg(vx).set(this->m_timer);
        }

        template <uint8_t vx>
        void t_set_i_font() noexcept {
            this->m_i_register = this->reg(vx).value() * 5;
        }

        template <uint16_t address>
        void t_call() noexcept {
            this->m_stack.push(this->m_ip);
            this->m_ip = address;
        }

        template <uint16_t address>
        void t_set_i() noexcept {
            this->m_i_register = address;
        }

        template <uint16_t address>
        void t_long_jump() noexcept {
            this->m_ip = address + this->reg(0).value();
        }

        uint16_t fetch() noexcept {
            const auto value = static_cast<uint16_t>(this->m_memory[this->m_ip]) << 8 | this->m_memory[this->m_ip + 1];
            this->m_ip += 2;
            return value;
        }

    private:
        std::array<Register, RegisterCount> m_registers{};
        FrontEndManager& m_manager;
        Display m_display;
        StaticVector<uint16_t, 48> m_stack{};
        std::vector<uint8_t> m_memory{};
        uint16_t m_ip{ 0x200 };
        uint16_t m_i_register{ 0x0 };
        uint8_t m_timer{ 0 };
        size_t m_ips{ 10000 };
        size_t m_target_fps{ 60 };
        bool m_high_clock{ true };
    };
} // cip