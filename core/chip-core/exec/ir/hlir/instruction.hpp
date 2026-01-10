#pragma once
#include "chip-core/cpu_info.hpp"
#include "register.hpp"
#include "util/enum.hpp"
#include "util/types.hpp"

#include <variant>
#include <vector>

namespace cip::hlir {
    enum class InstructionType {
        Mov,
        GetCPUMemory,
        GetDisplayMemory,
        And,
        Add,
        Mul,
        Shr,
        Xor,
        JmpGEBlock,
        JmpLBlock,
        JmpBlock,
        JmpZBlock,
        UpCastReg,
        DownCastReg,
        ReadPtr,
        WritePtr,
        ReturnToJit
    };

    enum class RegisterModificationType : u8 {
        Write = 1,
        Read = 2,
    };

    struct RegisterModificationInfo {
        Register reg{};
        RegisterModificationType modification_type{};
    };

    class Instruction {
    public:
        explicit Instruction(const InstructionType type) : m_type{ type } {}
        virtual ~Instruction() = default;

        virtual std::vector<RegisterModificationInfo> modification_info() const = 0;

    protected:
        InstructionType m_type{};
    };

    template <InstructionType Type>
    struct BaseInstruction : Instruction {
        BaseInstruction() : Instruction(Type) {}
        ~BaseInstruction() override = default;
    };

    template <InstructionType Type>
    class BlockControlFlowInstruction : public BaseInstruction<Type> {
    protected:
        using Instr = BlockControlFlowInstruction;
        struct RegWiseCompare {
            Register target;
            Register compared;
        };

        struct ImmWiseCompare {
            Register target;
            u64 immediate;
        };

    public:
        BlockControlFlowInstruction() = default;
        BlockControlFlowInstruction(Register target, Register compared, const u32 block_index)
            : m_block_index(block_index), m_compare_info{
                  RegWiseCompare{ target, compared }
        } {}

        BlockControlFlowInstruction(Register target, u64 immediate, const u32 block_index)
            : m_block_index(block_index), m_compare_info{
                  ImmWiseCompare{ target, immediate }
        } {}

        std::vector<RegisterModificationInfo> modification_info() const override {
            if (std::holds_alternative<ImmWiseCompare>(this->m_compare_info)) {
                const auto& comp = std::get<ImmWiseCompare>(this->m_compare_info);

                auto access = std::vector{
                    RegisterModificationInfo{ .reg = comp.target, .modification_type = RegisterModificationType::Read }
                };

                return access;
            }

            const auto& comp = std::get<RegWiseCompare>(this->m_compare_info);

            auto access = std::vector{
                RegisterModificationInfo{ .reg = comp.target,   .modification_type = RegisterModificationType::Read  },
                RegisterModificationInfo{ .reg = comp.compared, .modification_type = RegisterModificationType::Write }
            };
            return access;
        }

    protected:
        u32 m_block_index{};
        std::variant<RegWiseCompare, ImmWiseCompare> m_compare_info{};
    };

    template <InstructionType Type>
    class BinaryInstruction : public BaseInstruction<Type> {
    protected:
        using Instr = BinaryInstruction;
        struct BinaryImm {
            Register dst;
            u64 immediate{};
        };

        struct BinaryReg {
            Register dst;
            Register src;
        };

    public:
        BinaryInstruction() = default;
        BinaryInstruction(const Register dst, const u64 immediate)
            : m_instruction_state(BinaryImm{ dst, immediate }) {};
        BinaryInstruction(const Register dst, const Register src) : m_instruction_state(BinaryReg{ dst, src }) {
            assert(!(dst.is_ptr() && src.is_ptr())); // One component cannot be a pointer
            if (dst.bit_width() < 32) {
                assert(dst.bit_width() == src.bit_width());
            } else {
                assert(dst.bit_width() >= 32 && src.bit_width() >= 32);
            }
        }

        std::vector<RegisterModificationInfo> modification_info() const override {
            if (std::holds_alternative<BinaryReg>(this->m_instruction_state)) {
                const auto& comp = std::get<BinaryReg>(this->m_instruction_state);

                auto access = std::vector{
                    RegisterModificationInfo{ .reg = comp.dst,
                                             .modification_type =
                                                  RegisterModificationType::Read | RegisterModificationType::Write },
                    RegisterModificationInfo{ .reg = comp.src, .modification_type = RegisterModificationType::Read }
                };

                return access;
            }

            const auto& comp = std::get<BinaryImm>(this->m_instruction_state);
            auto access = std::vector{
                RegisterModificationInfo{ .reg = comp.dst,
                                         .modification_type =
                                              RegisterModificationType::Write | RegisterModificationType::Read }
            };
            return access;
        }

    protected:
        std::variant<BinaryImm, BinaryReg> m_instruction_state;
    };

    struct ReturnToJit : BaseInstruction<InstructionType::ReturnToJit> {
        std::vector<RegisterModificationInfo> modification_info() const override { return {}; }
    };

    struct Mov : BinaryInstruction<InstructionType::Mov> {
        using Instr::Instr;
        std::vector<RegisterModificationInfo> modification_info() const override {
            if (std::holds_alternative<BinaryReg>(this->m_instruction_state)) {
                const auto& comp = std::get<BinaryReg>(this->m_instruction_state);

                auto access = std::vector{
                    RegisterModificationInfo{ .reg = comp.dst, .modification_type = RegisterModificationType::Write },
                    RegisterModificationInfo{ .reg = comp.src, .modification_type = RegisterModificationType::Read  }
                };

                return access;
            }

            const auto& comp = std::get<BinaryImm>(this->m_instruction_state);
            auto access = std::vector{
                RegisterModificationInfo{ .reg = comp.dst, .modification_type = RegisterModificationType::Write }
            };
            return access;
        }
    };

    struct And : BinaryInstruction<InstructionType::And> {
        using Instr::Instr;
    };

    struct Add : BinaryInstruction<InstructionType::Add> {
        using Instr::Instr;
    };

    struct Mul : BinaryInstruction<InstructionType::Mul> {
        using Instr::Instr;
    };

    struct Xor : BinaryInstruction<InstructionType::Xor> {
        using Instr::Instr;
    };

    struct Shr : BinaryInstruction<InstructionType::Shr> {
        using Instr::Instr;
    };

    struct JmpGEBlock : BlockControlFlowInstruction<InstructionType::JmpGEBlock> {
        using Instr::Instr;
    };

    struct JmpLBlock : BlockControlFlowInstruction<InstructionType::JmpLBlock> {
        using Instr::Instr;
    };

    template <InstructionType Type>
    class BaseJump : public BaseInstruction<Type> {
    public:
        using Instr = BaseJump;

        explicit BaseJump(const u32 block_index) : m_block_index(block_index) {}
        std::vector<RegisterModificationInfo> modification_info() const override { return {}; }

    protected:
        u32 m_block_index{};
    };

    struct JmpBlock : BaseJump<InstructionType::JmpBlock> {
        using Instr::Instr;
    };

    class JmpZBlock : public BaseJump<InstructionType::JmpZBlock> {
    public:
        JmpZBlock(const Register test_reg, const u32 dst_block) : BaseJump(dst_block), m_test_point(test_reg) {}

    private:
        Register m_test_point{};
    };

    class DownCastReg : BaseInstruction<InstructionType::DownCastReg> {
    public:
        DownCastReg() = default;
        DownCastReg(const Register dst, const Register src) : m_dst(dst), m_src(src) {
            assert(dst.bit_width() <= src.bit_width());
        }

        std::vector<RegisterModificationInfo> modification_info() const override {
            return std::vector{
                RegisterModificationInfo{ .reg = this->m_dst, .modification_type = RegisterModificationType::Write },
                RegisterModificationInfo{ .reg = this->m_src, .modification_type = RegisterModificationType::Read  }
            };
        }

    private:
        Register m_dst;
        Register m_src;
    };
    template <InstructionType Type, RegisterModificationType target_modification>
    class PtrInstruction : public BaseInstruction<Type> {
    protected:
        using Instr = PtrInstruction;
        struct ImmReadPtr {
            Register m_target{};
            Register m_ptr{};
            i32 m_offset{};
        };

        struct RegImmReadPtr {
            Register m_target{};
            Register m_reg_offset{};
            Register m_ptr{};
            i32 m_offset{};
            i32 m_scale{};
        };

    public:
        PtrInstruction(const Register target, const Register ptr, const i32 offset = 0)
            : m_instr_data(ImmReadPtr{ target, ptr, offset }) {
            assert(ptr.bit_width() == sizeof(uintptr_t) * 8);
            assert(ptr.is_ptr());
        }

        PtrInstruction(
            const Register target, const Register ptr, const Register reg_offset, const i32 scale = 1,
            const i32 offset = 0
        )
            : m_instr_data(RegImmReadPtr{ target, reg_offset, ptr, offset, scale }) {
            assert(ptr.bit_width() == sizeof(uintptr_t) * 8);
            assert(reg_offset.bit_width() == sizeof(uintptr_t) * 8 || reg_offset.bit_width() == sizeof(uintptr_t) * 4);
            assert(ptr.is_ptr());
        }

        std::vector<RegisterModificationInfo> modification_info() const override {
            if (std::holds_alternative<ImmReadPtr>(this->m_instr_data)) {
                const auto& comp = std::get<ImmReadPtr>(this->m_instr_data);
                return std::vector{
                    RegisterModificationInfo{ .reg = comp.m_target, .modification_type = target_modification            },
                    RegisterModificationInfo{ .reg = comp.m_ptr,    .modification_type = RegisterModificationType::Read }
                };
            }

            const auto& comp = std::get<RegImmReadPtr>(this->m_instr_data);
            return std::vector{
                RegisterModificationInfo{ .reg = comp.m_target,     .modification_type = target_modification            },
                RegisterModificationInfo{ .reg = comp.m_ptr,        .modification_type = RegisterModificationType::Read },
                RegisterModificationInfo{ .reg = comp.m_reg_offset,
                                         .modification_type = RegisterModificationType::Read                            },
            };
        }

    protected:
        std::variant<ImmReadPtr, RegImmReadPtr> m_instr_data;
    };

    struct ReadPtr : PtrInstruction<InstructionType::ReadPtr, RegisterModificationType::Write> {
        using Instr::Instr;
    };

    struct WritePtr : PtrInstruction<InstructionType::WritePtr, RegisterModificationType::Read> {
        using Instr::Instr;
    };

    class UpCastReg : public BaseInstruction<InstructionType::UpCastReg> {
    public:
        UpCastReg() = default;
        UpCastReg(const Register dst, const Register src) : m_dst(dst), m_src(src) {
            assert(dst.bit_width() >= src.bit_width());
        }

        std::vector<RegisterModificationInfo> modification_info() const override {
            return std::vector{
                RegisterModificationInfo{ .reg = this->m_dst, .modification_type = RegisterModificationType::Write },
                RegisterModificationInfo{ .reg = this->m_src, .modification_type = RegisterModificationType::Read  }
            };
        }

    private:
        Register m_dst;
        Register m_src;
    };

    class GetCPUMemory : public BaseInstruction<InstructionType::GetCPUMemory> {
    public:
        explicit GetCPUMemory(const Register dst) : m_dst{ dst } { assert(dst.bit_width() == sizeof(uintptr_t) * 8); }

        std::vector<RegisterModificationInfo> modification_info() const override {
            return std::vector{
                RegisterModificationInfo{ .reg = this->m_dst, .modification_type = RegisterModificationType::Write }
            };
        }

    private:
        Register m_dst{};
    };

    class GetDisplayMemory : public BaseInstruction<InstructionType::GetDisplayMemory> {
    public:
        explicit GetDisplayMemory(const Register dst) : m_dst{ dst } {
            assert(dst.bit_width() == sizeof(uintptr_t) * 8);
        }

        std::vector<RegisterModificationInfo> modification_info() const override {
            return std::vector{
                RegisterModificationInfo{ .reg = this->m_dst, .modification_type = RegisterModificationType::Write }
            };
        }

    private:
        Register m_dst{};
    };

} // namespace cip::hlir
