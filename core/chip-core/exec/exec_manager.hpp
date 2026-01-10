#pragma once
#include "util/memory_stream.hpp"
#include "util/types.hpp"

#include <stdexcept>
#include <unordered_map>

namespace cip {

    class ExecManager {
    public:
        ExecManager() = default;
        ExecManager(const ExecManager&) = delete;
        ExecManager& operator=(const ExecManager&) = delete;
        ExecManager(ExecManager&&) = delete;
        ExecManager& operator=(ExecManager&&) = delete;

        void execute_from(u16 program_counter, const MemoryStream& prog_mem);

    private:
        struct JitBlock {
            void execute() { throw std::logic_error("Unimplemented pathway"); }
        };

        std::unordered_map<u16, JitBlock> m_compiled_blocks;
    };

} // namespace cip
