#pragma once
#include "core_state.hpp"
#include "exec/exec_manager.hpp"

#include <atomic>
#include <span>

namespace cip {

    class ChipCore : public CoreState {
    public:
        void load_rom(std::span<const u8> memory, u16 start_pc);

        void execute();

        void set_running(bool value);

    private:
        std::atomic_bool m_running{ true };
        ExecManager m_exec_manager{};
    };

} // namespace cip
