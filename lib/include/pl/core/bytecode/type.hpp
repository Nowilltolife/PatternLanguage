#pragma once

#include <pl/core/token.hpp>
#include <pl/core/bytecode/symbol.hpp>
#include <pl/helpers/types.hpp>

namespace pl::core::instr {

    using Operand = u16;

    struct TypeInfo {
        enum TypeId : Operand {
            /* Builtin types */
            U8, U16, U24, U32, U48, U64, U128,
            S8, S16, S24, S32, S48, S64, S128,
            Bool, Float, Double, Char, Char16,
            String, Padding, Auto, CustomType,
            /* Complex types */
            Structure, Union, Enum, Bitfield,
        };

        TypeId id;
        SymbolId name;

        [[nodiscard]] constexpr inline static bool isBuiltin(TypeId id) {
            return id <= TypeId::Char16;
        }

        [[nodiscard]] constexpr inline static bool isComplex(TypeId id) {
            return id >= TypeId::Structure;
        }

        [[nodiscard]] constexpr inline static bool isSigned(TypeId id) {
            return id >= TypeId::S8 && id <= TypeId::S128;
        }

        [[nodiscard]] constexpr inline static bool isUnsigned(TypeId id) {
            return id <= TypeId::U128;
        }

        [[nodiscard]] constexpr inline static bool isInteger(TypeId id) {
            return isSigned(id) || isUnsigned(id);
        }

        [[nodiscard]] static TypeId fromLiteral(Token::ValueType type) {
            switch (type) {
                case Token::ValueType::Unsigned8Bit: return TypeId::U8;
                case Token::ValueType::Unsigned16Bit: return TypeId::U16;
                case Token::ValueType::Unsigned24Bit: return TypeId::U24;
                case Token::ValueType::Unsigned32Bit: return TypeId::U32;
                case Token::ValueType::Unsigned48Bit: return TypeId::U48;
                case Token::ValueType::Unsigned64Bit: return TypeId::U64;
                case Token::ValueType::Unsigned128Bit: return TypeId::U128;
                case Token::ValueType::Signed8Bit: return TypeId::S8;
                case Token::ValueType::Signed16Bit: return TypeId::S16;
                case Token::ValueType::Signed24Bit: return TypeId::S24;
                case Token::ValueType::Signed32Bit: return TypeId::S32;
                case Token::ValueType::Signed48Bit: return TypeId::S48;
                case Token::ValueType::Signed64Bit: return TypeId::S64;
                case Token::ValueType::Signed128Bit: return TypeId::S128;
                case Token::ValueType::Boolean: return TypeId::Bool;
                case Token::ValueType::Float: return TypeId::Float;
                case Token::ValueType::Double: return TypeId::Double;
                case Token::ValueType::Character: return TypeId::Char;
                case Token::ValueType::Character16: return TypeId::Char16;
                case Token::ValueType::String: return TypeId::String;
                case Token::ValueType::Auto: return TypeId::Auto;
                case Token::ValueType::CustomType: return TypeId::CustomType;
                default: return TypeId::Auto;
            }
        }

        [[nodiscard]] static u8 getTypeSize(TypeId id) {
            switch (id) {
                case TypeId::U8: return 1;
                case TypeId::U16: return 2;
                case TypeId::U24: return 3;
                case TypeId::U32: return 4;
                case TypeId::U48: return 6;
                case TypeId::U64: return 8;
                case TypeId::U128: return 16;
                case TypeId::S8: return 1;
                case TypeId::S16: return 2;
                case TypeId::S24: return 3;
                case TypeId::S32: return 4;
                case TypeId::S48: return 6;
                case TypeId::S64: return 8;
                case TypeId::S128: return 16;
                case TypeId::Bool: return 1;
                case TypeId::Float: return 4;
                case TypeId::Double: return 8;
                case TypeId::Char: return 1;
                case TypeId::Char16: return 2;
                default: return 0;
            }
        }

        [[nodiscard]] static std::string getTypeName(TypeId id) {
            switch (id) {
                case TypeId::U8: return "u8";
                case TypeId::U16: return "u16";
                case TypeId::U24: return "u24";
                case TypeId::U32: return "u32";
                case TypeId::U48: return "u48";
                case TypeId::U64: return "u64";
                case TypeId::U128: return "u128";
                case TypeId::S8: return "s8";
                case TypeId::S16: return "s16";
                case TypeId::S24: return "s24";
                case TypeId::S32: return "s32";
                case TypeId::S48: return "s48";
                case TypeId::S64: return "s64";
                case TypeId::S128: return "s128";
                case TypeId::Bool: return "bool";
                case TypeId::Float: return "float";
                case TypeId::Double: return "double";
                case TypeId::Char: return "char";
                case TypeId::Char16: return "char16";
                case TypeId::String: return "string";
                case TypeId::Padding: return "padding";
                case TypeId::Auto: return "auto";
                case TypeId::CustomType: return "custom";
                case TypeId::Structure: return "struct";
                case TypeId::Union: return "union";
                case TypeId::Enum: return "enum";
                case TypeId::Bitfield: return "bitfield";
                default: return "unknown";
            }
        }

    };


}
