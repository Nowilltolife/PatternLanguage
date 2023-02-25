#pragma once
#include <map>
#include <vector>
#include <stack>

#include <pl/core/bytecode/bytecode.hpp>

namespace pl::core::vm {

    enum ValueType {
        BOOLEAN,
        BYTE,
        UNSIGNED_INTEGER,
        SIGNED_INTEGER,
        SCALAR,
        ARRAY,
        FIELD,
        STRUCTURE,
        POINTER
    };

    struct Value;

    struct Attribute {
        u16 name;
    };

    struct Pattern {
        u16 name;
        u16 typeName;
        u64 location;
        u64 section;
        std::map<u16, Attribute> attributes;
    };

    struct Field : Pattern {
        Value *value{};
    };

    struct Struct : Pattern {
        std::map<u16, Field> fields;
    };

    struct Value {
        u16 size;
        u8 type;
        union {
            bool boolean;
            u8 byte;
            u128 unsignedInteger;
            i128 signedInteger;
            double scalar;
            Value *array;
            Field *field;
            Struct *structure;
            Value *pointer;
        };
    };

    class VirtualMachine {
    public:
        VirtualMachine();
        void run();
        void loadBytecode(instr::Bytecode bytecode);
        Value* executeFunction(const std::string function);

    private:
        void step();
        void accessData(u64 address, void *buffer, size_t size, u64 sectionId, bool write);
        void readData(u64 address, void *buffer, size_t size, u64 sectionId) {
            this->accessData(address, buffer, size, sectionId, false);
        }
        void enterFunction(u32 name);
        void leaveFunction();

        struct Frame {
            std::map<u32, Value*> locals;
            std::stack<Value*> stack;
            u64 pc;
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

        u64 m_address;
        Frame* m_frame;
        StaticNames m_staticNames;
        std::stack<Frame*> m_frames;
        std::stack<Value*> m_stack;
        instr::SymbolTable m_symbolTable;
        std::vector<instr::Instruction*> m_instructions;
        std::vector<instr::Bytecode::Function> m_functions;
        bool m_running;
    };
}