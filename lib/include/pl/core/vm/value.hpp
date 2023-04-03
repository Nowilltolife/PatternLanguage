#pragma once
#include <map>
#include <vector>
#include <stack>

#include <pl/core/bytecode/symbol.hpp>
#include <pl/core/bytecode/bytecode.hpp>

namespace pl::core {

    class VirtualMachine; // forward declare to mark vm as friend

    using SymbolId = instr::SymbolId;

    namespace impl {
        struct ValueImpl;
    }

    using Value = std::shared_ptr<impl::ValueImpl>;

    struct Attribute {
        SymbolId name;

        auto operator<=>(const Attribute&) const = default;
    };

    struct Object {
        SymbolId name;
        SymbolId typeName;
        u128 address;
        u64 section;
        u32 color;
        std::map<SymbolId, Attribute> attributes;

        auto operator<=>(const Object&) const = default;
    };

    struct Field : Object {
        Value value;
    };

    struct TypedValue {
        Value value;
        SymbolId type;
    };

    struct Struct : Object {
        std::map<SymbolId, Field> fields; // name -> field
    };

    struct StaticArray {
        Value templateValue; // template value is also start, it contains base size, section and address
        SymbolId type; // type of array
        u16 size; // size of array, final size = base size * size

        auto operator<=>(const StaticArray&) const = default;
    };

    struct DynamicArray {
        std::vector<Value> values; // contained values
        SymbolId type; // type of array

        auto operator<=>(const DynamicArray&) const = default;
    };

    using Val = std::variant<bool, u128, i128, double, Value, Field*, Struct, StaticArray, DynamicArray>;

    namespace impl {
        struct ValueImpl {
            friend class pl::core::VirtualMachine;
        public:
            /**
             * Size of this value
             */
            u16 size{};
            /**
             * Address of this value
             */
            u128 address{};
            /**
             * Section of this value
             */
            u64 section{};
#ifdef DEBUG
            /**
             * Type of this value, only available in debug mode
             */
            instr::TypeInfo::TypeId type{};
#endif

            /**
             * Assign a new value to hold
             * @param value new value
             */
            void inline setValue(auto value) {
                this->m_value = value;
            }

            /**
             * Convert this value to a unsigned integer
             * @return unsigned integer
             */
            [[nodiscard]] u128 toUnsigned() const {
                return primitiveVisit<u128>("integer");
            }

            /**
             * Convert this value to a signed integer
             * @return signed integer
             */
             [[nodiscard]] i128 toSigned() const {
                return primitiveVisit<i128>("integer");
            }

            /**
             * Convert this value to a floating point number
             * @return floating point number
             */
            [[nodiscard]] double toFloatingPoint() const {
                return primitiveVisit<double>("floating point");
            }

            /**
             * Convert this value to a boolean
             * @return
             */
            [[nodiscard]] bool toBoolean() const {
                return primitiveVisit<bool>("boolean");
            }

            /**
             * Convert this value to a struct reference
             * @return struct reference, @c nullptr if this value is not a struct
             */
            [[nodiscard]] inline Struct* toStruct() {
                return std::get_if<Struct>(&this->m_value);
            }

            /**
             * Convert this value to a field reference
             * @return field reference, @c nullptr if this value is not a field
             */
            [[nodiscard]] inline Field** toField() {
                return std::get_if<Field*>(&this->m_value);
            }

            /**
             * Convert this value to a static array reference
             * @return static array reference, @c nullptr if this value is not a static array
             */
            [[nodiscard]] inline StaticArray* toStaticArray() {
                return std::get_if<StaticArray>(&this->m_value);
            }

            /**
             * Convert this value to a dynamic array reference
             * @return dynamic array reference, @c nullptr if this value is not a dynamic array
             */
            [[nodiscard]] inline DynamicArray* toDynamicArray() {
                return std::get_if<DynamicArray>(&this->m_value);
            }

            /**
             * Determine if this value is an integer
             * @return @c true if this value is an integer, @c false otherwise
             */
            [[nodiscard]] inline bool isInteger() const {
                return std::holds_alternative<u128>(this->m_value) || std::holds_alternative<i128>(this->m_value);
            }

            /**
             * Format this value to a string
             * @param table symbol table
             * @param indent indentation
             * @param recursionDepth recursion depth
             * @return formatted string
             */
            [[nodiscard]] std::string format(const instr::SymbolTable &table, u64 indent = 0, u64 recursionDepth = 0) const {
                if (recursionDepth > 5) {
                    recursionDepth--;
                    return "...";
                }
                recursionDepth++;
                const auto &b = std::visit([table, recursionDepth, &indent](auto &&arg) -> std::string {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, bool>) {
                        return arg ? "true" : "false";
                    } else if constexpr (std::is_same_v<T, u128> || std::is_same_v<T, i128>) {
                        return fmt::format("0x{:X}", arg);
                    } else if constexpr (std::is_same_v<T, double>) {
                        return fmt::format("{}", arg);
                    } else if constexpr (std::is_same_v<T, Value>) {
                        return arg->format(table);
                    } else if constexpr (std::is_same_v<T, Field *>) {
                        return arg->value->format(table);
                    } else if constexpr (std::is_same_v<T, Struct>) {
                        if (arg.fields.empty()) return "{}";
                        std::string result = "{\n";

                        indent++;
                        for (auto &[name, field]: arg.fields) {
                            result += fmt::format("{}{}: {},\n", std::string(indent, ' '),
                                                  table.getString(field.name),
                                                  field.value->format(table, indent, recursionDepth));
                        }
                        indent--;

                        result += std::string(indent, ' ') + "}";

                        return result;
                    } else if constexpr (std::is_same_v<T, StaticArray>) {
                        if (arg.size == 0) return "{}";

                        return fmt::format("{{ T: {}, S: {} }}", arg.templateValue->format(table, indent, recursionDepth), arg.size);
                    } else if constexpr (std::is_same_v<T, DynamicArray>) {
                        if (arg.values.empty()) return "[]";
                        std::string result = "[ ";

                        for (auto &value: arg.values) {
                            result += fmt::format("{}{}, ", std::string(indent, ' '), value->format(table, indent, recursionDepth));
                        }

                        result += " ]";

                        return result;
                    } else {
                        return "unknown";
                    }
                }, m_value);
                recursionDepth--;
                return b;
            }

        private:
            template <typename T>
            inline T primitiveVisit(std::string_view t) const {
                return std::visit(wolv::util::overloaded {
                    [&t](const Value& value) -> T { return value->primitiveVisit<T>(t); },
                    [&t](Field *field) -> T { return field->value->primitiveVisit<T>(t); },
                    [&t](const Struct&) -> T { err::E0004.throwError(fmt::format("Cannot cast value to type '{}'", t)); },
                    [&t](const StaticArray&) -> T { err::E0004.throwError(fmt::format("Cannot cast value to type '{}'", t)); },
                    [&t](const DynamicArray&) -> T { err::E0004.throwError(fmt::format("Cannot cast value to type '{}'", t)); },
                    [](auto &&result) -> T { return result; }
                }, m_value);
            }

        protected:
            Val m_value;
        };
    }

    Value newValue();

};