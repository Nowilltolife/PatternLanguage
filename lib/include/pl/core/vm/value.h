#pragma once
#include <map>
#include <vector>
#include <stack>

#include <pl/core/bytecode/bytecode.hpp>

namespace pl::core {

    struct Value_;

    typedef std::shared_ptr<Value_> Value;

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

        bool operator==(const Object& other) const {
            return this->name == other.name && this->typeName == other.typeName && this->location == other.location && this->section == other.section;
        }
    };

    struct Field : Object {
        Value value;
    };

    struct Struct : Object {
        std::map<u16, Field> fields;
    };

    struct Value_ {
        u16 size{};
        u128 address{};
        u64 section{};

        struct Internal : public std::variant<bool, u128, i128, double, Value, Field*, Struct> {
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

            std::string format(const instr::SymbolTable& table) {
                return std::visit([table](auto&& arg) -> std::string {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, bool>) {
                        return arg ? "true" : "false";
                    } else if constexpr (std::is_same_v<T, u128> || std::is_same_v<T, i128>) {
                        return fmt::format("0x{:X}", arg);
                    }else if constexpr (std::is_same_v<T, double>) {
                        return fmt::format("{}", arg);
                    } else if constexpr (std::is_same_v<T, Value>) {
                        return arg->v.format(table);
                    } else if constexpr (std::is_same_v<T, Field*>) {
                        return arg->value->v.format(table);
                    } else if constexpr (std::is_same_v<T, Struct>) {
                        if(arg.fields.empty()) return "{}";
                        std::string result = "{\n";

                        for (auto &[name, field] : arg.fields) {
                            result += fmt::format(" {}: {},\n", table.getString(field.name), field.value->v.format(table));
                        }

                        result += "}";

                        return result;
                    }
                    return "unknown";
                }, *this);
            }
        } v;
    };

};