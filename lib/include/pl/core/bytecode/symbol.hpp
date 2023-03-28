#pragma once
#include <vector>

#include <pl/helpers/types.hpp>

namespace pl::core::instr {

    using SymbolId = u16;

    enum class SymbolType {
        STRING,
        UNSIGNED_INTEGER,
        SIGNED_INTEGER,
    };

    struct Symbol {
        SymbolType type{};

        virtual ~Symbol() = default;
        [[nodiscard]] virtual std::string toString() const = 0;
        [[nodiscard]] virtual u64 hash() const = 0;
    };

    struct StringSymbol : Symbol {
        explicit StringSymbol(std::string string) : value(std::move(string)) {
            this->type = SymbolType::STRING;
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
            this->type = SymbolType::UNSIGNED_INTEGER;
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
            this->type = SymbolType::SIGNED_INTEGER;
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

        [[nodiscard]] SymbolId newString(const std::string& str) {
            auto symbol = new StringSymbol { str };
            return this->addSymbol(symbol);
        }

        [[nodiscard]] SymbolId newUnsignedInteger(u64 value) {
            auto symbol = new UISymbol { value };
            return this->addSymbol(symbol);
        }

        [[nodiscard]] SymbolId newSignedInteger(i64 value) {
            auto symbol = new SISymbol { value };
            return this->addSymbol(symbol);
        }

        [[nodiscard]] SymbolId addSymbol(Symbol *symbol) {
            // see if symbol already exists
            for (SymbolId i = 1; i < m_symbols.size(); i++) {
                if (m_symbols[i]->hash() == symbol->hash()) {
                    delete symbol;
                    return i;
                }
            }
            auto result = m_symbols.size();
            m_symbols.push_back(symbol);
            return result;
        }

        [[nodiscard]] inline Symbol *getSymbol(SymbolId index) const {
            return m_symbols[index];
        }

        [[nodiscard]] const std::string& getString(SymbolId index) const {
            return static_cast<StringSymbol*>(this->getSymbol(index))->value;
        }

        [[nodiscard]] u64 getUnsignedInteger(SymbolId index) const {
            return static_cast<UISymbol*>(this->getSymbol(index))->value;
        }

        [[nodiscard]] i64 getSignedInteger(SymbolId index) const {
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
}