#pragma once

#include <string>

#include <pl/helpers/types.hpp>
#include <utility>

namespace pl::core::instr {

    constexpr static const char * const this_name = "<this>";
    constexpr static const char * const ctor_name = "<init>";
    constexpr static const char * const main_name = "<main>";

    enum Opcode {
        STORE_FIELD,
        LOAD_FIELD,
        STORE_IN_THIS,
        LOAD_FROM_THIS,
        STORE_ATTRIBUTE,
        STORE_LOCAL,
        LOAD_LOCAL,
        NEW_STRUCT,
        READ_VALUE,
        READ_FIELD,
        CALL,
        EXPORT,
        DUP,
        RETURN
    };

    constexpr static const char * const opcodeNames[] = {
        "store_field",
        "load_field",
        "store_in_this",
        "load_from_this",
        "store_attribute",
        "store_local",
        "load_local",
        "new_struct",
        "read_value",
        "read_field",
        "call",
        "export",
        "dup",
        "return"
    };

    struct Instruction {
        Opcode opcode;
        std::vector<u16> operands{};
    };

    enum SymbolType {
        STRING
    };

    struct Symbol {
        u16 type{};

        virtual ~Symbol() = default;
        [[nodiscard]] virtual std::string toString() const = 0;
        [[nodiscard]] virtual u64 hash() const = 0;
    };

    struct StringSymbol : Symbol {
        explicit StringSymbol(std::string string) : value(std::move(string)) {
            this->type = STRING;
        }

        std::string value;

        ~StringSymbol() override = default;
        [[nodiscard]] std::string toString() const override {
            return this->value;
        }

        [[nodiscard]] u64 hash() const override {
            return std::hash<std::string>{}(this->value);
        }
    };

    class SymbolTable {
    public:
        SymbolTable() : m_symbols() {
            this->m_symbols.push_back(nullptr); // increase index by 1
        }

        SymbolTable(const SymbolTable &other) = default;

        SymbolTable(SymbolTable &&other) noexcept {
            this->m_symbols = std::move(other.m_symbols);
        }

        SymbolTable &operator=(const SymbolTable &other) = default;

        SymbolTable &operator=(SymbolTable &&other) noexcept {
            this->m_symbols = std::move(other.m_symbols);
            return *this;
        }

        ~SymbolTable() = default;

        [[nodiscard]] u16 newString(const std::string& str) {
            auto symbol = new StringSymbol { str };
            return this->addSymbol(symbol);
        }

        [[nodiscard]] u16 addSymbol(Symbol *symbol) {
            // see if symbol already exists
            for (u16 i = 1; i < m_symbols.size(); i++) {
                if (m_symbols[i]->hash() == symbol->hash()) {
                    delete symbol;
                    return i;
                }
            }
            auto result = m_symbols.size();
            m_symbols.push_back(symbol);
            return result;
        }

        [[nodiscard]] Symbol *getSymbol(u16 index) const {
            return m_symbols[index];
        }

    private:
        std::vector<Symbol*> m_symbols;
    };

    class BytecodeEmitter {
    public:

        BytecodeEmitter(SymbolTable& symbolTable, std::vector<Instruction>* instructions)
            : m_symbolTable(symbolTable), m_instructions(instructions) { }

        void store_field(const std::string& name, const std::string& typeName, bool slot0 = false) {
            hlp::unused(this->addInstruction({
                                                     slot0 ? STORE_IN_THIS : STORE_FIELD,
                                                     {
                                                             this->m_symbolTable.newString(name),
                                                             this->m_symbolTable.newString(typeName)
                                                     }
                                             }));
        }

        void load_field(const std::string& name, bool slot0 = false) {
            hlp::unused(this->addInstruction({
                                                     slot0 ? LOAD_FROM_THIS : LOAD_FIELD,
                                                     {
                                                             this->m_symbolTable.newString(name),
                                                     }
                                             }));
        }


        void store_local(const std::string& name, const std::string& typeName) {
            hlp::unused(this->addInstruction({
                                                     STORE_LOCAL,
                                                     {
                                                             this->m_symbolTable.newString(name),
                                                             this->m_symbolTable.newString(typeName)
                                                     }
                                             }));
        }

        void load_local(const std::string& name) {
            hlp::unused(this->addInstruction({ LOAD_LOCAL, { this->m_symbolTable.newString(name) } }));
        }

        void store_attribute(const std::string& name) {
            hlp::unused(this->addInstruction({ STORE_ATTRIBUTE, { this->m_symbolTable.newString(name) } }));
        }

        void read_value(u16 type) {
            hlp::unused(this->addInstruction({ READ_VALUE, { type }}));
        }

        void read_field(const std::string& name, const std::string& typeName, u16 type) {
            hlp::unused(this->addInstruction({
                                                     READ_FIELD,
                                                     {
                                                             this->m_symbolTable.newString(name),
                                                             this->m_symbolTable.newString(typeName),
                                                             type
                                                     }
                                             }));
        }

        void new_struct(const std::string& name) {
            hlp::unused(this->addInstruction({ NEW_STRUCT, { this->m_symbolTable.newString(name) } }));
        }

        void call(const std::string& name) {
            hlp::unused(this->addInstruction({ CALL, { this->m_symbolTable.newString(name) } }));
        }

        void export_() {
            hlp::unused(this->addInstruction({ EXPORT }));
        }

        void dup() {
            hlp::unused(this->addInstruction({ DUP }));
        }

        void return_() {
            hlp::unused(this->addInstruction({ RETURN }));
        }

        [[nodiscard]]

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
    private:
        SymbolTable& m_symbolTable;
        std::vector<Instruction>* m_instructions;
    };

    class Bytecode {
    public:

        struct Function {
            u16 name;
            std::vector<Instruction>* instructions;
        };

        struct StructInfo {
            u16 name;
            u64 size; // size of all fields added up
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

        [[nodiscard]] const SymbolTable &getSymbolTable() {
            return m_symbolTable;
        }

        [[nodiscard]] const std::vector<Function>& getFunctions() const {
            return m_functions;
        }

        std::string toString() const {
            std::string ss;
            for (auto& function : m_functions) {
                ss += "function " + m_symbolTable.getSymbol(function.name)->toString() + " {\n";
                for (auto& instruction : *function.instructions) {
                    ss += "    " + std::string(opcodeNames[instruction.opcode]) + " ";
                    switch (instruction.opcode) {
                        case STORE_FIELD:
                        case LOAD_FIELD:
                        case STORE_ATTRIBUTE:
                        case LOAD_LOCAL:
                        case NEW_STRUCT:
                        case LOAD_FROM_THIS:
                        case CALL: {
                            u16 index = instruction.operands[0];
                            ss += '#' + std::to_string(index) + " (" + m_symbolTable.getSymbol(index)->toString() + ")";
                            break;
                        }
                        case READ_VALUE: {
                            auto t = static_cast<Token::ValueType>(instruction.operands[0]);

                            ss += std::to_string(instruction.operands[0]) + " (" + Token::getTypeName(t) + ")";
                            break;
                        }
                        case READ_FIELD: {
                            u16 index1 = instruction.operands[0];
                            u16 index2 = instruction.operands[1];
                            auto t = static_cast<Token::ValueType>(instruction.operands[2]);

                            ss += '#' + std::to_string(index1) + " (" + m_symbolTable.getSymbol(index1)->toString() + "), ";
                            ss += '#' + std::to_string(index2) + " (" + m_symbolTable.getSymbol(index2)->toString() + "), ";
                            ss += std::to_string(instruction.operands[2]) + " (" + Token::getTypeName(t) + ")";
                            break;
                        }
                        case STORE_IN_THIS:
                        case STORE_LOCAL: {
                            u16 index1 = instruction.operands[0];
                            u16 index2 = instruction.operands[1];
                            ss += '#' + std::to_string(index1) + " (" + m_symbolTable.getSymbol(index1)->toString() + "), ";
                            ss += '#' + std::to_string(index2) + " (" + m_symbolTable.getSymbol(index2)->toString() + ")";
                            break;
                        }
                        default:
                            break;
                    }
                    ss += '\n';
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