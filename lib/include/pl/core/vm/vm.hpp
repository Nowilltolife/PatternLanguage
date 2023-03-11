#pragma once
#include <map>
#include <vector>
#include <stack>

#include <pl/core/bytecode/bytecode.hpp>

namespace pl::core::vm {

    template<typename T>
    static std::unique_ptr<T> create(const std::string &typeName, const std::string &varName, auto... args) {
        auto pattern = std::make_unique<T>(nullptr, args...);
        pattern->setTypeName(typeName);
        pattern->setVariableName(varName);

        return std::move(pattern);
    }

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

    struct Object {
        u16 name;
        u16 typeName;
        u64 location;
        u64 section;
        u32 color;
        std::map<u16, Attribute> attributes;
    };

    struct Field : Object {
        Value *value{};
    };

    struct Struct : Object {
        std::map<u16, Field> fields;
    };

    struct Value {
        u16 size{};
        u128 address{};
        u64 section{};

        struct Internal : public std::variant<bool, u128, i128, double, Value*, Field*, Struct*> {
            using variant::variant;

            u128 toInteger() {
                return std::visit([](auto&& arg) -> u128 {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, bool>) {
                        return arg ? 1 : 0;
                    } else if constexpr (std::is_same_v<T, u128> || std::is_same_v<T, i128>) {
                        return (u128) arg;
                    } else {
                        err::E0002.throwError("Cannot convert value to integer", {}, 0);
                    }
                }, *this);
            }

            bool toBool() {
                return std::visit([](auto&& arg) -> bool {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, bool>) {
                        return arg;
                    } else if constexpr (std::is_same_v<T, u128> || std::is_same_v<T, i128>) {
                        return arg != 0;
                    } else {
                        err::E0002.throwError("Cannot convert value to bool", {}, 0);
                    }
                }, *this);
            }

            std::string format() {
                return std::visit([](auto&& arg) -> std::string {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, bool>) {
                        return arg ? "true" : "false";
                    } else if constexpr (std::is_same_v<T, u128>) {
                        return fmt::format("0x{:X}", arg);
                    } else if constexpr (std::is_same_v<T, i128>) {
                        return fmt::format("0x{:X}", arg);
                    } else if constexpr (std::is_same_v<T, double>) {
                        return fmt::format("{}", arg);
                    } else if constexpr (std::is_same_v<T, Value*>) {
                        return arg->v.format();
                    } else if constexpr (std::is_same_v<T, Field*>) {
                        return arg->value->v.format();
                    } else if constexpr (std::is_same_v<T, Struct*>) {
                        return "struct";
                    }
                    return "unknown";
                }, *this);
            }
        } v;
    };

    class VirtualMachine {
    public:

        struct IOOperations {
            std::function<void(u64, u8*, size_t)> read;
            std::function<void(u64, u8*, size_t)> write;
        };

        void run();
        void loadBytecode(instr::Bytecode bytecode);
        Value* executeFunction(const std::string& function);

        void setIOOperations(const IOOperations& ioOperations) {
            this->m_io = ioOperations;
        }

        [[nodiscard]] auto& getPatterns() const {
            return this->m_patterns;
        }

    private:
        void step();
        void accessData(u64 address, void *buffer, size_t size, u64 sectionId, bool write);
        void readData(u64 address, void *buffer, size_t size, u64 sectionId) {
            this->accessData(address, buffer, size, sectionId, false);
        }
        void enterFunction(u32 name);
        void leaveFunction();
        Value* readStaticValue(u16 type);

        struct Frame {
            std::map<u32, Value*> locals;
            std::stack<Value*> stack;
            std::vector<instr::Instruction>* m_instructions;
            u64 pc;

            ~Frame() {
                for (auto& [key, value] : this->locals) {
                    delete value;
                }
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

        std::shared_ptr<ptrn::Pattern> convert(Value* value);

        std::string lookupString(u32 name) {
            auto symbol = this->m_symbolTable.getSymbol(name);
            auto stringSymbol = dynamic_cast<instr::StringSymbol*>(symbol);
            if(stringSymbol == nullptr)
                return "<invalid>";
            return stringSymbol->value;
        }

        u64 m_address;
        Frame* m_frame;
        StaticNames m_staticNames;
        std::stack<Frame*> m_frames;
        std::stack<Value*> m_stack;
        instr::SymbolTable m_symbolTable;
        std::vector<instr::Bytecode::Function> m_functions;
        std::vector<std::shared_ptr<ptrn::Pattern>> m_patterns;
        Value* result;
        bool m_running;
        IOOperations m_io;

        enum Condition {
            EQUAL,
            NOT_EQUAL,
            LESS,
            LESS_EQUAL,
            GREATER,
            GREATER_EQUAL
        };

        static Value* compare(Value* b, Value* a, Condition condition);
    };
}