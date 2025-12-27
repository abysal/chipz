#include "instruction_list.hpp"

#include <optional>
#include <stdexcept>
#include <utility>

namespace jip {
    bool Instruction::is_skip_next() const noexcept {
        switch (this->m_type) {
        case InstructionType::SkipEqRegImm:
        case InstructionType::SkipNeRegImm:
        case InstructionType::SkipNeRegReg:
        case InstructionType::SkipEqRegReg:
        case InstructionType::SkipKeyDown:
        case InstructionType::SkipKeyUp:
            return true;
        default:
            return false;
        }
    }

    void InstructionList::create_block(MemoryStream stream, const uint16_t ip) {
        auto current_ip = ip;

        // Plan:
        // If the instruction skips the next instruction
        // We peek into that instruction and check if it's a jump operation.
        // If that instr is a jump, we check if it's a self jump and note that the jump is a self jump.
        // If the jump is a jump which breaks the bounds of the current block, we note it is a JIT jump which needs to
        // be handled by the JIT
        //
        // If the instruction is a control flow changer
        // We check if it's a jump which jumps into our own block, if so we note it and end the block (infinite loop
        // detected) if it's not a jump which jumps into our own block we also note it and end the current block
        //
        // If the instruction is "normal" we just consume it
        while (true) {
            const auto instr = this->decode_instruction(stream.next_word());
            current_ip += 2;
            if (!instr.has_value()) {
                break;
            }
            const auto& inst = *instr;

            if (!inst.is_skip_next()) {

                if (inst.type() == InstructionType::Jump) {
                    // Check for self jumps
                    const auto target = instr->immediate();
                    if (target <= current_ip && target >= ip) {
                        // We know this jumps into a spot we recognise
                        this->m_local_jump_points.emplace_back(target);
                    }
                    this->m_instructions.emplace_back(instr.value());
                    break;
                }

                this->m_instructions.emplace_back(instr.value());
                if (inst.changes_control_flow()) {
                    // Means it's a call or a return
                    break;
                }
            } else {
                this->m_instructions.emplace_back(instr.value());
                if (!stream.has_next()) {
                    // we can assume some kind of SMC possibly is happening
                    break;
                }

                const auto next_instr = this->decode_instruction(stream.next_word());
                if (!next_instr.has_value()) {
                    break; // Again, assume SMC and bork the block
                }
                const auto& next_inst = *next_instr;
                current_ip += 2;

                this->m_local_jump_points.emplace_back(current_ip); // Skip the current instruction
                if (next_inst.changes_control_flow()) {
                    if (next_inst.type() == InstructionType::Jump) {
                        const auto target = next_inst.immediate();
                        if (target <= current_ip || target >= ip) {
                            this->m_local_jump_points.emplace_back(target);
                        }
                    }
                }

                this->m_instructions.emplace_back(next_inst);
            }
        }
    }

    std::optional<Instruction> InstructionList::decode_instruction(uint16_t bytes) {
        using RegVec = cip::StaticVector<uint8_t, 2, uint8_t>;
        const auto type = compute_type(bytes);
        if (type == InstructionType::Invalid) {
            return std::nullopt;
        }

        const auto instr_info = [bytes, type] {
            switch (type) {
            case InstructionType::Native:
            case InstructionType::Jump:
            case InstructionType::Call:
            case InstructionType::LoadImmI:
            case InstructionType::LongJump:
                return std::make_tuple(0, RegVec{}, bytes & 0xFFF);
            case InstructionType::SkipEqRegImm:
            case InstructionType::SkipNeRegImm:
            case InstructionType::LoadImm:
            case InstructionType::AddImm:
            case InstructionType::Random:
                return std::make_tuple(1, RegVec{ static_cast<uint8_t>((bytes & 0xF00) >> 8) }, bytes & 0xFF);
            case InstructionType::MovReg:
            case InstructionType::RegOr:
            case InstructionType::RegAnd:
            case InstructionType::RegXor:
            case InstructionType::RegAddYX:
            case InstructionType::RegSubYX:
            case InstructionType::RegShrXY:
            case InstructionType::RegSubXY:
            case InstructionType::RegShlXY:
            case InstructionType::SkipNeRegReg:
            case InstructionType::SkipEqRegReg:
                return std::make_tuple(
                    2,
                    RegVec{
                        { static_cast<uint8_t>((bytes & 0xF00) >> 8), static_cast<uint8_t>((bytes & 0xF0) >> 4) }
                },
                    0
                );
            case InstructionType::Draw:
                return std::make_tuple(
                    2,
                    RegVec{ static_cast<uint8_t>((bytes & 0xF00) >> 8), static_cast<uint8_t>((bytes & 0xF0) >> 4) },
                    bytes & 0xF
                );
            case InstructionType::SkipKeyDown:
            case InstructionType::SkipKeyUp:
            case InstructionType::WaitKeyPress:
            case InstructionType::LoadDelayReg:
            case InstructionType::LoadRegDelay:
            case InstructionType::SetSoundReg:
            case InstructionType::IAddReg:
            case InstructionType::LoadFont:
            case InstructionType::BCD:
            case InstructionType::RangeRead:
            case InstructionType::RangeWrite:
                return std::make_tuple(1, RegVec{ static_cast<uint8_t>((bytes & 0xF00) >> 8) }, 0);
            case InstructionType::Invalid:
                throw std::runtime_error("Invalid instruction detected");
            }
            std::unreachable();
        }();

        const auto control_flow_change = [type, bytes] {
            switch (type) {
            case InstructionType::Jump:

            case InstructionType::Call:

            case InstructionType::LongJump:

            case InstructionType::SkipEqRegImm:

            case InstructionType::SkipNeRegImm:

            case InstructionType::SkipNeRegReg:

            case InstructionType::SkipEqRegReg:

            case InstructionType::SkipKeyDown:
            case InstructionType::SkipKeyUp:
                return true;
            case InstructionType::Native:
                return bytes == 0x00EE;
            default:
                return false;
            }
        }();

        Instruction instr{
            static_cast<uint8_t>(std::get<0>(instr_info)),  control_flow_change, type, std::get<1>(instr_info),
            static_cast<uint16_t>(std::get<2>(instr_info)),
        };

        return instr;
    }
} // namespace jip