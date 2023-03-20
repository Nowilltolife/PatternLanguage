#pragma once
#include <map>
#include <vector>
#include <stack>

#include "pl/core/bytecode/bytecode.hpp"
#include "pl/core/vm/value.h"

namespace pl::core {

    template<typename T>
    static std::unique_ptr<T> create(const std::string &typeName, const std::string &varName, auto... args) {
        auto pattern = std::make_unique<T>(nullptr, args...);
        pattern->setTypeName(typeName);
        pattern->setVariableName(varName);

        return std::move(pattern);
    }

    enum BitfieldOrder {
        LSB,
        MSB
    };

    struct VMSettings {
        std::endian defaultEndian = std::endian::native;
        BitfieldOrder defaultBitfieldOrder = BitfieldOrder::LSB;
    };

    class VirtualMachine {
    public:

        struct IOOperations {
            std::function<void(u64, u8*, size_t)> read;
            std::function<void(u64, const u8*, size_t)> write;
        };

        void run();
        void step();
        void enterMain();
        void loadBytecode(instr::Bytecode bytecode);
        Value executeFunction(const std::string& function);

        void setIOOperations(const IOOperations& ioOperations) {
            this->m_io = ioOperations;
        }

        [[nodiscard]] inline auto& getPatterns() const {
            return this->m_patterns;
        }

        [[nodiscard]] inline auto& getFrame() const {
            return this->m_frame;
        }

        [[nodiscard]] inline auto& getSymbols() const {
            return this->m_symbolTable;
        }

        void reset();

        void readData(u64 address, void *buffer, size_t size, u64 sectionId) {
            this->accessData(address, buffer, size, sectionId, false);
        }

        void writeData(u64 address, const void *buffer, size_t size, u64 sectionId) {
            this->accessData(address, const_cast<void*>(buffer), size, sectionId, true);
        }

        void abort() {
            this->m_running = false;
        }

    public:
        [[nodiscard]] std::endian getDefaultEndian() const { return this->m_settings.defaultEndian; }
        void setDefaultEndian(std::endian endian) { this->m_settings.defaultEndian = endian; }

        [[nodiscard]] BitfieldOrder getDefaultBitfieldOrder() const { return this->m_settings.defaultBitfieldOrder; }
        void setDefaultBitfieldOrder(BitfieldOrder order) { this->m_settings.defaultBitfieldOrder = order; }

        [[nodiscard]] const std::endian& getEndian() const { return this->m_endian; }
        void setEndian(std::endian endian) { this->m_endian = endian; }

        [[nodiscard]] const BitfieldOrder& getBitfieldOrder() const { return this->m_bitfieldOrder; }
        void setBitfieldOrder(BitfieldOrder order) { this->m_bitfieldOrder = order; }

        [[nodiscard]] LogConsole& getConsole() { return this->m_console; }

        u64& dataOffset() { return this->m_address; }

        [[nodiscard]] std::optional<u64> getCurrentArrayIndex() const { return this->m_currentArrayIndex; }
        void setCurrentArrayIndex(u64 index) { this->m_currentArrayIndex = index; }

        u64 getDataBaseAddress() const { return this->m_dataBaseAddress; }
        void setDataBaseAddress(u64 address) { this->m_dataBaseAddress = address; }

        u64 getDataSize() const { return this->m_dataSize; }
        void setDataSize(u64 size) { this->m_dataSize = size; }

    private:
        void accessData(u64 address, void *buffer, size_t size, u64 sectionId, bool write);
        void enterFunction(u32 name);
        void leaveFunction();
        Value readStaticValue(u16 type);

        template <typename T>
        class Stack : public std::deque<T> {
        public:
            void push(T value) {
                this->push_back(value);
            }

            T pop() {
                auto value = this->back();
                this->pop_back();
                return value;
            }

            T top() {
                return this->back();
            }
        };

        struct Frame {
            std::map<u32, Value> locals;
            Stack<Value> stack;
            std::vector<instr::Instruction>* m_instructions;
            u64 pc;

            ~Frame() {
                locals.clear();
                stack.clear();
            }
        };

        struct StaticNames {
            u32 thisName;
            u32 mainName;
            u32 globalName;
        };

        instr::Bytecode::Function lookupFunction(u32 name) {
            for (auto& function : this->m_functions) {
                if (function.name == name) {
                    return function;
                }
            }
            return {};
        }

        std::shared_ptr<ptrn::Pattern> convert(const Value& value);

        std::string lookupString(u32 name) {
            auto symbol = this->m_symbolTable.getSymbol(name);
            auto stringSymbol = dynamic_cast<instr::StringSymbol*>(symbol);
            if(stringSymbol == nullptr)
                return "<invalid>";
            return stringSymbol->value;
        }

        u64 m_address;
        u64 m_dataSize;
        u64 m_dataBaseAddress;
        std::optional<u64> m_currentArrayIndex;
        Frame* m_frame;
        StaticNames m_staticNames;
        Stack<Frame*> m_frames;
        instr::SymbolTable m_symbolTable;
        std::vector<instr::Bytecode::Function> m_functions;
        instr::Bytecode::Function m_function;
        std::vector<std::shared_ptr<ptrn::Pattern>> m_patterns;
        std::endian m_endian;
        BitfieldOrder m_bitfieldOrder;
        VMSettings m_settings;
        Value result;
        bool m_running;
        IOOperations m_io;
        LogConsole m_console;

        enum Condition {
            EQUAL,
            NOT_EQUAL,
            LESS,
            LESS_EQUAL,
            GREATER,
            GREATER_EQUAL
        };

        static Value compare(Value b, Value a, Condition condition);
    };
}