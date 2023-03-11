#pragma once

#include <string>

#include <pl/helpers/types.hpp>
#include <utility>

namespace pl::core::instr {

    constexpr static const char * const this_name = "<this>";
    constexpr static const char * const ctor_name = "<init>";
    constexpr static const char * const main_name = "<main>";

    enum Opcode {
        STORE_FIELD, LOAD_FIELD,
        STORE_IN_THIS, LOAD_FROM_THIS,
        STORE_ATTRIBUTE, STORE_LOCAL,
        LOAD_LOCAL, NEW_STRUCT,
        READ_VALUE, READ_FIELD,
        LOAD_SYMBOL, CALL,
        EXPORT, DUP,
        EQ, NEQ,
        GT, GTE,
        LT, LTE,
        AND, OR,
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
        "export", "dup",
        "eq", "neq",
        "gt", "gte",
        "lt", "lte",
        "and", "or",
        "not", "cmp",
        "jmp", "return"
    };

    struct Instruction {
        Opcode opcode;
        std::vector<u16> operands{};
    };

    enum SymbolType {
        STRING,
        UNSIGNED_INTEGER,
        SIGNED_INTEGER,
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

    struct UISymbol : Symbol {
        explicit UISymbol(u64 value) : value(value) {
            this->type = UNSIGNED_INTEGER;
        }

        u64 value;

        ~UISymbol() override = default;
        [[nodiscard]] std::string toString() const override {
            return std::to_string(this->value);
        }

        [[nodiscard]] u64 hash() const override {
            return std::hash<u64>{}(this->value);
        }
    };

    class SISymbol : public Symbol {
    public:
        explicit SISymbol(i64 value) : value(value) {
            this->type = SIGNED_INTEGER;
        }

        i64 value;

        ~SISymbol() override = default;
        [[nodiscard]] std::string toString() const override {
            return std::to_string(this->value);
        }

        [[nodiscard]] u64 hash() const override {
            return std::hash<i64>{}(this->value);
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

        [[nodiscard]] u16 newUnsignedInteger(u64 value) {
            auto symbol = new UISymbol { value };
            return this->addSymbol(symbol);
        }

        [[nodiscard]] u16 newSignedInteger(i64 value) {
            auto symbol = new SISymbol { value };
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
        struct EmitterFlags {
            bool main : 1 = false;
            bool ctor : 1 = false;
        };

        struct Label {
            u16 targetPc;
            std::vector<std::pair<u16, u16>> targets;
        };

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

        void load_symbol(u16 index) {
            hlp::unused(this->addInstruction({ LOAD_SYMBOL, { index } }));
        }

        void new_struct(const std::string& name) {
            hlp::unused(this->addInstruction({ NEW_STRUCT, { this->m_symbolTable.newString(name) } }));
        }

        void call(const std::string& name) {
            hlp::unused(this->addInstruction({ CALL, { this->m_symbolTable.newString(name) } }));
        }

        void export_(const std::string& name) {
            hlp::unused(this->addInstruction({ EXPORT , { this->m_symbolTable.newString(name) } }));
        }

        void dup() {
            hlp::unused(this->addInstruction({ DUP }));
        }

        void cmp() {
            hlp::unused(this->addInstruction({ CMP }));
        }

        void eq() {
            hlp::unused(this->addInstruction({ EQ }));
        }

        void neq() {
            hlp::unused(this->addInstruction({ NEQ }));
        }

        void lt() {
            hlp::unused(this->addInstruction({ LT }));
        }

        void gt() {
            hlp::unused(this->addInstruction({ GT }));
        }

        void lte() {
            hlp::unused(this->addInstruction({ LTE }));
        }

        void gte() {
            hlp::unused(this->addInstruction({ GTE }));
        }

        void and_() {
            hlp::unused(this->addInstruction({ AND }));
        }

        void or_() {
            hlp::unused(this->addInstruction({ OR }));
        }

        void not_() {
            hlp::unused(this->addInstruction({ NOT }));
        }

        void jmp(Label& label) {
            label.targets.emplace_back( this->m_instructions->size(), 0 );
            hlp::unused(this->addInstruction({ JMP, { label.targetPc } }));
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
            hlp::unused(this->addInstruction({ RETURN }));
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

        [[nodiscard]] SymbolTable &getSymbolTable() {
            return m_symbolTable;
        }

        [[nodiscard]] const std::vector<Function>& getFunctions() const {
            return m_functions;
        }

        std::string insnToString(u16 pc, const Instruction& instruction, const Function& function) const {
            std::string ss = std::to_string(pc) + ": " + std::string(opcodeNames[instruction.opcode]) + " ";
            switch (instruction.opcode) {
                case STORE_FIELD:
                case LOAD_FIELD:
                case STORE_ATTRIBUTE:
                case LOAD_LOCAL:
                case NEW_STRUCT:
                case LOAD_FROM_THIS:
                case LOAD_SYMBOL:
                case CALL: {
                    u16 index = instruction.operands[0];
                    return ss + '#' + std::to_string(index) + " (" + m_symbolTable.getSymbol(index)->toString() + ")";
                }
                case JMP: {
                    i16 index = (i16) instruction.operands[0];
                    u16 target = pc + index;
                    auto insn = function.instructions->at(target);
                    return ss + (index > 0 ? '+' : '-') + std::to_string(index) + " (" + insnToString(target, insn, function) + ")";
                }
                case READ_VALUE: {
                    auto t = static_cast<Token::ValueType>(instruction.operands[0]);

                    return ss + std::to_string(instruction.operands[0]) + " (" + Token::getTypeName(t) + ")";
                }
                case READ_FIELD: {
                    u16 index1 = instruction.operands[0];
                    u16 index2 = instruction.operands[1];
                    auto t = static_cast<Token::ValueType>(instruction.operands[2]);

                    ss += '#' + std::to_string(index1) + " (" + m_symbolTable.getSymbol(index1)->toString() + "), ";
                    ss += '#' + std::to_string(index2) + " (" + m_symbolTable.getSymbol(index2)->toString() + "), ";
                    return ss + std::to_string(instruction.operands[2]) + " (" + Token::getTypeName(t) + ")";
                }
                case STORE_IN_THIS:
                case STORE_LOCAL: {
                    u16 index1 = instruction.operands[0];
                    u16 index2 = instruction.operands[1];
                    ss += '#' + std::to_string(index1) + " (" + m_symbolTable.getSymbol(index1)->toString() + "), ";
                    return ss + '#' + std::to_string(index2) + " (" + m_symbolTable.getSymbol(index2)->toString() + ")";
                    break;
                }
                case EXPORT: {
                    u16 index = instruction.operands[0];
                    return ss + '#' + std::to_string(index) + " (" + m_symbolTable.getSymbol(index)->toString() + ")";
                }
                default:
                    return ss;
            }
        }

        std::string toString() const {
            std::string ss;
            for (const auto& function : m_functions) {
                ss += "function " + m_symbolTable.getSymbol(function.name)->toString() + " {\n";
                u16 pc = 0;
                for (const auto& instruction : *function.instructions) {
                    ss += "    " + insnToString(pc, instruction, function) + "\n";
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