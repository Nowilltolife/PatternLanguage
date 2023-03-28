#pragma once

#include <string>
#include <utility>

#include <pl/core/bytecode/type.hpp>
#include <pl/core/bytecode/symbol.hpp>
#include <pl/helpers/types.hpp>
#include <pl/core/token.hpp>

namespace pl::core::instr {

    constexpr static const char * const this_name = "this";
    constexpr static const char * const ctor_name = "<init>";
    constexpr static const char * const main_name = "<main>";

    enum Opcode {
        STORE_FIELD, LOAD_FIELD,
        STORE_IN_THIS, LOAD_FROM_THIS,
        STORE_ATTRIBUTE, STORE_LOCAL,
        LOAD_LOCAL, NEW_STRUCT,
        READ_VALUE, READ_FIELD,
        LOAD_SYMBOL, CALL,
        EXPORT, DUP, POP,
        READ_ARRAY,
        EQ, NEQ,
        GT, GTE,
        LT, LTE,
        NOT, CMP,
        JMP, RETURN
    };

    constexpr static const char * const opcodeNames[] = {
        "store_field", "load_field",
        "store_in_this", "load_from_this",
        "store_attribute", "store_local",
        "load_local", "new_struct",
        "read_value", "read_field",
        "load_symbol", "call",
        "export", "dup", "pop",
        "read_array",
        "eq", "neq",
        "gt", "gte",
        "lt", "lte",
        "not", "cmp",
        "jmp", "return"
    };

    using JumpOffset = i16;

    struct Instruction {
        Opcode opcode;
        std::vector<Operand> operands{};

        [[nodiscard]] std::string toString(const SymbolTable& symbols) const {
            std::string ss = std::string(opcodeNames[opcode]) + ' ';
            switch (opcode) {
                case STORE_FIELD:
                case LOAD_FIELD:
                case STORE_ATTRIBUTE:
                case LOAD_LOCAL:
                case NEW_STRUCT:
                case LOAD_FROM_THIS:
                case LOAD_SYMBOL:
                case CALL: {
                    SymbolId index = operands[0];
                    return ss + '#' + std::to_string(index) + " (" + symbols.getSymbol(index)->toString() + ")";
                }
                case JMP: {
                    auto index = (JumpOffset) operands[0];
                    return ss + (index > 0 ? '+' : '-') + std::to_string(index);
                }
                case READ_VALUE: {
                    auto id = static_cast<TypeInfo::TypeId>(operands[1]);

                    return ss + symbols.getString(operands[0]) + " (" + TypeInfo::getTypeName(id) + ")";
                }
                case READ_FIELD: {
                    u16 index1 = operands[0];
                    u16 index2 = operands[1];
                    auto id = static_cast<TypeInfo::TypeId>(operands[2]);

                    ss += '#' + std::to_string(index1) + " (" + symbols.getSymbol(index1)->toString() + "), ";
                    ss += '#' + std::to_string(index2) + " (" + symbols.getSymbol(index2)->toString() + ") ";
                    return ss + "(" + TypeInfo::getTypeName(id) + ")";
                }
                case STORE_IN_THIS:
                case STORE_LOCAL: {
                    u16 index1 = operands[0];
                    u16 index2 = operands[1];
                    ss += '#' + std::to_string(index1) + " (" + symbols.getSymbol(index1)->toString() + "), ";
                    return ss + '#' + std::to_string(index2) + " (" + symbols.getSymbol(index2)->toString() + ")";
                    break;
                }
                case EXPORT: {
                    u16 index = operands[0];
                    return ss + '#' + std::to_string(index) + " (" + symbols.getSymbol(index)->toString() + ")";
                }
                default:
                    return ss;
            }
        }
    };

    class BytecodeEmitter {
    public:
        struct EmitterFlags {
            bool main : 1 = false;
            bool ctor : 1 = false;
        };

        struct Label {
            u16 targetPc{};
            std::vector<std::pair<u16, u16>> targets;
        };

        BytecodeEmitter(SymbolTable& symbolTable, std::vector<Instruction>* instructions)
            : m_symbolTable(symbolTable), m_instructions(instructions) { }

        void store_field(const std::string& name, const std::string& typeName, bool slot0 = false) {
            wolv::util::unused(this->addInstruction({
                                                     slot0 ? STORE_IN_THIS : STORE_FIELD,
                                                     {
                                                             this->m_symbolTable.newString(name),
                                                             this->m_symbolTable.newString(typeName)
                                                     }
                                             }));
        }

        void load_field(const std::string& name, bool slot0 = false) {
            wolv::util::unused(this->addInstruction({
                                                     slot0 ? LOAD_FROM_THIS : LOAD_FIELD,
                                                     {
                                                             this->m_symbolTable.newString(name),
                                                     }
                                             }));
        }


        void store_local(const std::string& name, const std::string& typeName) {
            wolv::util::unused(this->addInstruction({
                                                     STORE_LOCAL,
                                                     {
                                                             this->m_symbolTable.newString(name),
                                                             this->m_symbolTable.newString(typeName)
                                                     }
                                             }));
        }

        void load_local(const std::string& name) {
            wolv::util::unused(this->addInstruction({ LOAD_LOCAL, { this->m_symbolTable.newString(name) } }));
        }

        void store_attribute(const std::string& name) {
            wolv::util::unused(this->addInstruction({ STORE_ATTRIBUTE, { this->m_symbolTable.newString(name) } }));
        }

        void read_value(TypeInfo info) {
            wolv::util::unused(this->addInstruction({ READ_VALUE, { info.name, info.id }}));
        }

        void read_array(TypeInfo info) {
            wolv::util::unused(this->addInstruction({ READ_ARRAY, { info.name, info.id }}));
        }

        void read_field(const std::string& name, TypeInfo info) {
            wolv::util::unused(this->addInstruction({
                                                     READ_FIELD,
                                                     {
                                                             this->m_symbolTable.newString(name),
                                                             info.name,
                                                             info.id
                                                     }
                                             }));
        }

        void load_symbol(u16 index) {
            wolv::util::unused(this->addInstruction({ LOAD_SYMBOL, { index } }));
        }

        void new_struct(const std::string& name) {
            wolv::util::unused(this->addInstruction({ NEW_STRUCT, { this->m_symbolTable.newString(name) } }));
        }

        void call(const std::string& name) {
            wolv::util::unused(this->addInstruction({ CALL, { this->m_symbolTable.newString(name) } }));
        }

        void export_(const std::string& name) {
            wolv::util::unused(this->addInstruction({ EXPORT , { this->m_symbolTable.newString(name) } }));
        }

        void dup() {
            wolv::util::unused(this->addInstruction({ DUP }));
        }

        void pop() {
            wolv::util::unused(this->addInstruction({ POP }));
        }

        void cmp() {
            wolv::util::unused(this->addInstruction({ CMP }));
        }

        void eq() {
            wolv::util::unused(this->addInstruction({ EQ }));
        }

        void neq() {
            wolv::util::unused(this->addInstruction({ NEQ }));
        }

        void lt() {
            wolv::util::unused(this->addInstruction({ LT }));
        }

        void gt() {
            wolv::util::unused(this->addInstruction({ GT }));
        }

        void lte() {
            wolv::util::unused(this->addInstruction({ LTE }));
        }

        void gte() {
            wolv::util::unused(this->addInstruction({ GTE }));
        }

        void not_() {
            wolv::util::unused(this->addInstruction({ NOT }));
        }

        void jmp(Label& label) {
            label.targets.emplace_back( this->m_instructions->size(), 0 );
            wolv::util::unused(this->addInstruction({ JMP, { label.targetPc } }));
        }

        auto label() {
            return Label();
        }

        void place_label(Label& label) {
            label.targetPc = this->m_instructions->size();
        }

        void resolve_label(Label& label) {
            for(auto &[index, operand] : label.targets) {
                this->m_instructions->at(index).operands[operand] = label.targetPc - index;
            }
        }

        void return_() {
            wolv::util::unused(this->addInstruction({ RETURN }));
        }

        void local(const std::string& name, const std::string& typeName) {
            this->m_locals[name] = typeName;
        }

        [[nodiscard]] const std::string& getLocalType(const std::string& name) const {
            return this->m_locals.at(name);
        }

        u16 addInstruction(const Instruction& instruction) {
            auto result = m_instructions->size();
            m_instructions->push_back(instruction);
            return result;
        }

        [[nodiscard]] const Instruction& getInstruction(u16 index) const {
            return m_instructions->at(index);
        }

        [[nodiscard]] SymbolTable &getSymbolTable() {
            return m_symbolTable;
        }

        [[nodiscard]] std::pair<TypeInfo, std::string> getTypeInfo(const std::shared_ptr<ast::ASTNode>& typeNode);

        EmitterFlags flags;

    private:
        SymbolTable& m_symbolTable;
        std::vector<Label> m_labels;
        std::vector<Instruction>* m_instructions;
        std::map<std::string, std::string> m_locals;
    };

    class Bytecode {
    public:

        struct Function {
            u16 name;
            std::vector<Instruction>* instructions;
        };

        Bytecode() = default;

        Bytecode(const Bytecode &other) = default;

        Bytecode(Bytecode &&other) noexcept = default;

        Bytecode &operator=(const Bytecode &other) = delete;

        Bytecode &operator=(Bytecode &&other) noexcept = default;

        ~Bytecode() = default;

        [[nodiscard]] BytecodeEmitter function(const std::string& name) {
            auto instructions = new std::vector<Instruction>();
            m_functions.push_back({m_symbolTable.newString(name), instructions});
            return {m_symbolTable, instructions};
        }

        [[nodiscard]] SymbolTable &getSymbolTable() {
            return m_symbolTable;
        }

        [[nodiscard]] const std::vector<Function>& getFunctions() const {
            return m_functions;
        }

        std::string toString() const {
            std::string ss;
            for (const auto& function : m_functions) {
                ss += "function " + m_symbolTable.getSymbol(function.name)->toString() + " {\n";
                u16 pc = 0;
                for (const auto& instruction : *function.instructions) {
                    ss += "    " + std::to_string(pc) + ": " + instruction.toString(m_symbolTable) + "\n";
                    pc++;
                }
                ss += "}\n";
            }
            return ss;
        }

    private:
        SymbolTable m_symbolTable;
        std::vector<Function> m_functions;
    };
}