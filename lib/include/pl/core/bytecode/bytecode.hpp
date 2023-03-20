#pragma once

#include <string>

#include <pl/helpers/types.hpp>
#include <pl/core/token.hpp>
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
        EXPORT, DUP, POP,

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
        "eq", "neq",
        "gt", "gte",
        "lt", "lte",
        "not", "cmp",
        "jmp", "return"
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

        [[nodiscard]] inline Symbol *getSymbol(u16 index) const {
            return m_symbols[index];
        }

        [[nodiscard]] const std::string& getString(u16 index) const {
            return static_cast<StringSymbol*>(this->getSymbol(index))->value;
        }

        [[nodiscard]] u64 getUnsignedInteger(u16 index) const {
            return static_cast<UISymbol*>(this->getSymbol(index))->value;
        }

        [[nodiscard]] i64 getSignedInteger(u16 index) const {
            return static_cast<SISymbol*>(this->getSymbol(index))->value;
        }

        void clear() {
            for (auto symbol : m_symbols) {
                delete symbol;
            }
            m_symbols.clear();
            m_symbols.push_back(nullptr);
        }

        bool empty() const {
            return m_symbols.size() == 1;
        }

    private:
        std::vector<Symbol*> m_symbols;
    };

    struct Instruction {
        Opcode opcode;
        std::vector<u16> operands{};

        std::string toString(const SymbolTable& symbols) const {
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
                    u16 index = operands[0];
                    return ss + '#' + std::to_string(index) + " (" + symbols.getSymbol(index)->toString() + ")";
                }
                case JMP: {
                    i16 index = (i16) operands[0];
                    return ss + (index > 0 ? '+' : '-') + std::to_string(index);
                }
                case READ_VALUE: {
                    auto t = static_cast<Token::ValueType>(operands[0]);

                    return ss + std::to_string(operands[0]) + " (" + Token::getTypeName(t) + ")";
                }
                case READ_FIELD: {
                    u16 index1 = operands[0];
                    u16 index2 = operands[1];
                    auto t = static_cast<Token::ValueType>(operands[2]);

                    ss += '#' + std::to_string(index1) + " (" + symbols.getSymbol(index1)->toString() + "), ";
                    ss += '#' + std::to_string(index2) + " (" + symbols.getSymbol(index2)->toString() + "), ";
                    return ss + std::to_string(operands[2]) + " (" + Token::getTypeName(t) + ")";
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

        void pop() {
            hlp::unused(this->addInstruction({ POP }));
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

        void not_() {
            hlp::unused(this->addInstruction({ NOT }));
        }

        void jmp(Label& label) {
            label.targets.emplace_back( this->m_instructions->size(), 0 );
            hlp::unused(this->addInstruction({ JMP, { label.targetPc } }));
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