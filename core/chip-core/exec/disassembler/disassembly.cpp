#include "disassembly.hpp"

#include <ranges>
#include <sstream>

namespace cip::chip {
    std::string Disassembly::as_string() const noexcept {
        std::stringstream result;
        for (const auto& [index, instr] : *this | std::views::enumerate) {
            result << std::hex << "0x" << this->m_start_pc + index * 2 << " | " << instr.get_instr_name() << ": "
                   << instr.operand_string() << "\n";
        }
        return result.str();
    }
} // namespace cip::chip