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
        STORE_GLOBAL,
        LOAD_GLOBAL,
        NEW_STRUCT,
        READ_FIELD,
        CALL,
        EXPORT
    };

    constexpr static const char * const opcodeNames[13] = {
        "store_field",
        "load_field",
        "store_in_this",
        "load_from_this",
        "store_attribute",
        "store_local",
        "load_local",
        "store_global",
        "load_global",
        "new_struct",
        "read_field",
        "call",
        "export"
    };

    struct Instruction {
        Opcode opcode;
    };

    struct IntOpInstruction : Instruction {
        u16 value;
    };

    struct BiIntOpInstruction : Instruction {
        u16 value1;
        u16 value2;
    };

    enum SymbolType {
        STRING
    };

    struct Symbol {
        u16 type;

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
        SymbolTable() = default;

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
            for (u16 i = 0; i < m_symbols.size(); i++) {
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

        BytecodeEmitter(SymbolTable& symbolTable, std::vector<Instruction*>* instructions)
            : m_symbolTable(symbolTable), m_instructions(instructions) { }

        void store_field(const std::string& name, bool slot0 = false) {
            auto instruction = new IntOpInstruction {{slot0 ? STORE_IN_THIS : STORE_FIELD}, this->m_symbolTable.newString(name) };
            hlp::unused(this->addInstruction(instruction));
        }

        void load_field(const std::string& name, bool slot0 = false) {
            auto instruction = new IntOpInstruction {{slot0 ? LOAD_FROM_THIS : LOAD_FIELD}, this->m_symbolTable.newString(name) };
            hlp::unused(this->addInstruction(instruction));
        }

        void store_attribute(const std::string& name) {
            auto instruction = new IntOpInstruction { {STORE_ATTRIBUTE}, this->m_symbolTable.newString(name) };
            hlp::unused(this->addInstruction(instruction));
        }

        void store_local(const std::string& name) {
            auto instruction = new IntOpInstruction { {STORE_LOCAL}, this->m_symbolTable.newString(name) };
            hlp::unused(this->addInstruction(instruction));
        }

        void load_local(const std::string& name) {
            auto instruction = new IntOpInstruction { {LOAD_LOCAL}, this->m_symbolTable.newString(name) };
            hlp::unused(this->addInstruction(instruction));
        }

        void store_global(const std::string& name) {
            auto instruction = new IntOpInstruction { {STORE_GLOBAL}, this->m_symbolTable.newString(name) };
            hlp::unused(this->addInstruction(instruction));
        }

        void load_global(const std::string& name) {
            auto instruction = new IntOpInstruction { {LOAD_GLOBAL}, this->m_symbolTable.newString(name) };
            hlp::unused(this->addInstruction(instruction));
        }

        void read_field(const std::string& name, u16 type) {
            auto instruction = new BiIntOpInstruction { {READ_FIELD}, this->m_symbolTable.newString(name), type };
            hlp::unused(this->addInstruction(instruction));
        }

        void new_struct(const std::string& name) {
            auto instruction = new IntOpInstruction { {NEW_STRUCT}, this->m_symbolTable.newString(name) };
            hlp::unused(this->addInstruction(instruction));
        }

        void call(const std::string& name) {
            auto instruction = new IntOpInstruction { {CALL}, this->m_symbolTable.newString(name) };
            hlp::unused(this->addInstruction(instruction));
        }

        void export_() {
            auto instruction = new Instruction { EXPORT };
            hlp::unused(this->addInstruction(instruction));
        }

        [[nodiscard]]

        u16 addInstruction(Instruction *instruction) {
            auto result = m_instructions->size();
            m_instructions->push_back(instruction);
            return result;
        }

        [[nodiscard]] Instruction *getInstruction(u16 index) const {
            return m_instructions->at(index);
        }

        [[nodiscard]] SymbolTable &getSymbolTable() {
            return m_symbolTable;
        }
    private:
        SymbolTable& m_symbolTable;
        std::vector<Instruction*>* m_instructions;
    };

    class Bytecode {
    public:

        struct Function {
            std::string name;
            std::vector<Instruction*>* instructions;
        };

        Bytecode() = default;

        Bytecode(const Bytecode &other) = delete;

        Bytecode(Bytecode &&other) noexcept = default;

        Bytecode &operator=(const Bytecode &other) = delete;

        Bytecode &operator=(Bytecode &&other) noexcept = default;

        ~Bytecode() = default;

        [[nodiscard]] BytecodeEmitter function(const std::string& name) {
            auto instructions = new std::vector<Instruction*>();
            m_functions.push_back({name, instructions});
            return {m_symbolTable, instructions};
        }

        [[nodiscard]] SymbolTable &getSymbolTable() {
            return m_symbolTable;
        }

        std::string toString() const {
            std::string ss;
            for (auto& function : m_functions) {
                ss += "function " + function.name + " {\n";
                for (auto& instruction : *function.instructions) {
                    ss += "    " + std::string(opcodeNames[instruction->opcode]) + " ";
                    switch (instruction->opcode) {
                        case STORE_FIELD:
                        case LOAD_FIELD:
                        case STORE_ATTRIBUTE:
                        case STORE_LOCAL:
                        case LOAD_LOCAL:
                        case STORE_GLOBAL:
                        case LOAD_GLOBAL:
                        case NEW_STRUCT:
                        case STORE_IN_THIS:
                        case LOAD_FROM_THIS:
                        case CALL: {
                            u16 index = static_cast<IntOpInstruction *>(instruction)->value;
                            Symbol *symbol = m_symbolTable.getSymbol(index);
                            ss += '#' + std::to_string(index) + " (" + symbol->toString() + ")";
                            break;
                        }
                        case READ_FIELD: {
                            u16 index1 = static_cast<BiIntOpInstruction *>(instruction)->value1;
                            u16 typeIndex = static_cast<BiIntOpInstruction *>(instruction)->value2;
                            Symbol *symbol1 = m_symbolTable.getSymbol(index1);

                            auto t = static_cast<Token::ValueType>(typeIndex);

                            ss += '#' + std::to_string(index1) + " (" + symbol1->toString() + "), ";
                            ss += std::to_string(typeIndex) + " (" + Token::getTypeName(t) + ")";
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