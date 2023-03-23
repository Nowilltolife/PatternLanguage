#include <variant>
#include <assert.h>

#include <pl/core/token.hpp>
#include "pl/core/vm.hpp"
#include <pl/patterns/pattern.hpp>
#include <pl/patterns/pattern_unsigned.hpp>
#include <pl/patterns/pattern_signed.hpp>
#include <pl/patterns/pattern_struct.hpp>
#include <pl/patterns/pattern_boolean.hpp>
#include <pl/patterns/pattern_float.hpp>
#include <pl/helpers/utils.hpp>

#if defined(__clang__)
#define ASSUME(expr) __builtin_assume(expr)
#elif defined(__GNUC__) && !defined(__ICC)
#define ASSUME(expr) if (expr) {} else { __builtin_unreachable(); }
#elif defined(_MSC_VER) || defined(__ICC)
#define ASSUME(expr) __assume(expr)
#endif

// likely and unlikely macros
#if defined(__clang__) || defined(__GNUC__)
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#endif

#if DEBUG
#define ASSERT(cond, message) assert(cond && message)
#else
#define ASSERT(cond, message) ASSUME(cond)
#endif

#define COMPARE_CASE(op, operation) \
    case op: { \
        stack.push(compare(stack.pop(), stack.pop(), operation)); \
        break; \
    }

using namespace pl;
using namespace pl::core;
using namespace pl::core;
using namespace pl::core::instr;
using namespace pl::ptrn;
using pl::core::instr::Opcode;
using std::get_if;

// helper type for the visitor #4
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

#define FRAME this->m_frame
#define STATIC(x) this->m_staticNames.x
#define LOCAL(x) FRAME->locals[x]
#define STACK FRAME->stack
#define THIS LOCAL(STATIC(thisName))
#define INSTRUCTION FRAME->m_instructions[FRAME->pc]

u32 colorIndex = 0;

u32 getNextPalletColor() {
    constexpr static std::array Palette = { 0x70B4771F, 0x700E7FFF, 0x702CA02C, 0x702827D6, 0x70BD6794, 0x704B568C, 0x70C277E3, 0x7022BDBC, 0x70CFBE17 };

    auto index = colorIndex;
    colorIndex = (colorIndex + 1) % Palette.size();

    return Palette[index];
}

Value newValue() {
    return std::make_unique<Value_>();
}

Value VirtualMachine::readStaticValue(u16 type) {
    auto valueType = (Token::ValueType) type;
    auto value = newValue();
    value->size = Token::getTypeSize(valueType);
    value->address = this->m_address;
    value->section = 0;
    switch (valueType) {
        default: {
            if(Token::isUnsigned(valueType)) {
                u128 v = 0;
                this->readData(this->m_address, &v, value->size, 0);
                value->v = v;
            } else if(Token::isSigned(valueType)) {
                i128 v = 0;
                this->readData(this->m_address, &v, value->size, 0);
                value->v = v;
            } else if (Token::isFloatingPoint(valueType)) {
                double v = 0;
                this->readData(this->m_address, &v, value->size, 0);
                value->v = v;
            } else {
                err::E0001.throwError(fmt::format("#{:x} read_value failed: invalid type", m_frame->pc));
            }
        }
    }
    return value;
}

std::shared_ptr<Pattern> VirtualMachine::convert(const Value& value) {
    auto a = std::visit(overloaded {
        [&](bool b) {
            wolv::util::unused(b);
            return (std::shared_ptr<Pattern>) std::make_shared<PatternBoolean>(nullptr, value->address);
        },
        [&](double d) {
            wolv::util::unused(d);
            return (std::shared_ptr<Pattern>) std::make_shared<PatternFloat>(nullptr, value->address, value->size);
        },
        [&](u128 v) {
            wolv::util::unused(v);
            return (std::shared_ptr<Pattern>) std::make_shared<PatternUnsigned>(nullptr, value->address, value->size);
        },
        [&](i128 v) {
            wolv::util::unused(v);
            return (std::shared_ptr<Pattern>) std::make_shared<PatternSigned>(nullptr, value->address, value->size);
        },
        [&](const Struct& strct) {
            std::vector<std::shared_ptr<Pattern>> members;
            u64 size = 0;
            for (const auto &[name, field]: strct.fields) {
                auto fieldPattern = convert(field.value);
                fieldPattern->setVariableName(lookupString(field.name));
                fieldPattern->setTypeName(lookupString(field.typeName));
                members.push_back(fieldPattern);
                size += fieldPattern->getSize();
            }
            auto pattern = std::make_shared<PatternStruct>(nullptr, value->address, size);
            pattern->setTypeName(lookupString(strct.typeName));
            pattern->setVariableName(lookupString(strct.name));
            pattern->setMembers(members);
            return (std::shared_ptr<Pattern>) pattern;
        },
        [&](Field* field) {
            auto fieldPattern = convert(field->value);
            fieldPattern->setVariableName(lookupString(field->name));
            fieldPattern->setTypeName(lookupString(field->typeName));
            return (std::shared_ptr<Pattern>) fieldPattern;
        },
        [&](const Value& value) {
            return (std::shared_ptr<Pattern>) convert(value);
        }
    }, value->v);
    a->setColor(getNextPalletColor());
    a->setVm(this);
    return a;
}

void VirtualMachine::loadBytecode(Bytecode bytecode) {
    this->reset();
    this->m_symbolTable = bytecode.getSymbolTable();
    // initalize statics
    this->m_staticNames = {
        .thisName = this->m_symbolTable.newString(this_name),
        .mainName = this->m_symbolTable.newString(main_name),
        .globalName = this->m_symbolTable.newString("main")
    };
    this->m_functions = bytecode.getFunctions();
    colorIndex = 0;
}

void VirtualMachine::enterFunction(u32 name) {
    // retrieve from function table
    auto function = this->lookupFunction(name);
    ASSERT(function.name != 0, fmt::format("Function with name {} not found", name).c_str());
    this->m_function = function;
    auto actualName = dynamic_cast<StringSymbol* >(this->m_symbolTable.getSymbol(function.name));
    auto prev = this->m_frame;
    auto frame = new Frame();
    frame->pc = 0;
    frame->m_instructions = function.instructions;
    if(actualName->value[0] == '<') { // is initializer
        if (prev != nullptr) {
            ASSERT(!prev->stack.empty(), "Initializer called without a structure");
            const auto& v = prev->stack.pop();
            frame->locals[this->m_staticNames.thisName] = v;
            frame->stack.push(v); // also leave it at the bottom of stack so return will return it
        }
    }
    // set new frame
    this->m_frame = frame;
    if(prev != nullptr)
        this->m_frames.push(prev); // push current frame, so that it acts as the 'previous' frame
}

void VirtualMachine::leaveFunction() {
    auto &stack = FRAME->stack;
    if(this->m_frames.empty()) {
        // shutdown vm
        this->m_running = false;
        if(!stack.empty())
            this->result = stack.pop();
        delete this->m_frame;
        this->m_frame = nullptr;
    } else {
        auto prev = this->m_frames.pop();
        if(!stack.empty())
            prev->stack.push(stack.pop()); // return value
        // delete current frame
        delete this->m_frame;
        this->m_frame = prev; // switch control over
    }
}

Value VirtualMachine::executeFunction(const std::string& function) {
    auto name = this->m_symbolTable.newString(function);
    auto fn = this->lookupFunction(name);
    if(fn.name == 0) {
        err::E0001.throwError(fmt::format("Function '{}' not found", function));
    }
    this->enterFunction(this->m_symbolTable.newString(function));
    this->run();
    return this->result;
}

void VirtualMachine::enterMain() {
    this->enterFunction(this->m_staticNames.mainName);
}

void VirtualMachine::run() {
    this->m_running = true;
    while(this->m_running) {
        this->step();
    }
}

void VirtualMachine::reset() {
    colorIndex = 0;
    this->m_frames.clear();
    delete this->m_frame;
    this->m_frame = nullptr;
    this->m_function = {};
    this->m_running = false;
    this->m_symbolTable.clear();
    this->m_functions.clear();
    this->m_staticNames = {};
    this->result = nullptr;
    this->m_address = 0;
    this->m_patterns.clear();
}

void VirtualMachine::step() {
    auto frame = this->m_frame;
    auto &stack = frame->stack;
    auto &locals = frame->locals;
    auto &instruction = (*frame->m_instructions)[frame->pc++];
    switch (instruction.opcode) {
        case STORE_IN_THIS: {
            auto &name = instruction.operands[0];
            auto &typeName = instruction.operands[1];
            auto value = stack.pop();
            auto v = get_if<Struct>(&locals[m_staticNames.thisName]->v);
            ASSERT(v != nullptr, "'this' is not a structure");
            auto &f = (*v).fields[name];
            f.value = value;
            f.name = name;
            f.typeName = typeName;
            break;
        }
        case LOAD_FROM_THIS: {
            auto &index = instruction.operands[0];
            auto v = get_if<Struct>(&locals[m_staticNames.thisName]->v);
            ASSERT(v != nullptr, "'this' is not a structure");
            stack.push((*v).fields[index].value);
            break;
        }
        case STORE_FIELD: {
            auto &name = instruction.operands[0];
            auto value = stack.pop();
            auto structure = stack.pop();
            auto v = get_if<Struct>(&structure->v);
            if(v == nullptr) {
                err::E0001.throwError(fmt::format("#{:x} store_field failed: structure is not a structure", frame->pc));
            }
            auto &f = (*v).fields[name];
            f.value = value;
            f.name = name;
            break;
        }
        case DUP: {
            ASSERT(!stack.empty(), "Cannot duplicate empty stack");
            stack.push(stack.top());
            break;
        }
        case POP: {
            ASSERT(!stack.empty(), "Cannot pop empty stack");
            stack.pop();
            break;
        }
        case LOAD_FIELD:
            break;
        case STORE_ATTRIBUTE:
            break;
        case STORE_LOCAL: {
            auto &index = instruction.operands[0];
            //auto &typeName = instruction.operands[1];
            locals[index] = stack.pop();
            break;
        }
        case LOAD_LOCAL: {
            auto &index = instruction.operands[0];
            if(!locals.contains(index))
                err::E0003.throwError(fmt::format("No variable named '{}' found.", lookupString(index)), {}, 0);
            stack.push(locals[index]);
            break;
        }
        case LOAD_SYMBOL: {
            auto &index = instruction.operands[0];
            auto symbol = this->m_symbolTable.getSymbol(index);
            auto value = newValue();
            switch (symbol->type) {
                case SymbolType::STRING: {
                    // TODO: strings
                    //value->v = dynamic_cast<StringSymbol*>(symbol)->value;
                    break;
                }
                case SymbolType::UNSIGNED_INTEGER: {
                    value->v = (u128) dynamic_cast<UISymbol*>(symbol)->value;
                    break;
                }
                case SymbolType::SIGNED_INTEGER: {
                    value->v = (i128) dynamic_cast<SISymbol*>(symbol)->value;
                    break;
                }
            }
            stack.push(value);
            break;
        }
        case NEW_STRUCT: {
            auto &name = instruction.operands[0];
            auto value = newValue();
            Struct structure = {};
            structure.typeName = name;
            structure.fields = {};
            value->v = structure;
            value->address = this->m_address;
            stack.push(value);
            break;
        }
        case READ_VALUE: {
            auto v = readStaticValue(instruction.operands[0]);
            stack.push(v);
            this->m_address += v->size;
            break;
        }
        case READ_FIELD: {
            auto &name = instruction.operands[0];
            auto &typeName = instruction.operands[1];
            auto v = get_if<Struct>(&locals[m_staticNames.thisName]->v);
            ASSERT(v != nullptr, "'this' is not a structure");
            auto value = readStaticValue(instruction.operands[2]);
            this->m_address += value->size;
            auto &f = (*v).fields[name];
            f.value = value;
            f.name = name;
            f.typeName = typeName;
            break;
        }
        COMPARE_CASE(EQ, Condition::EQUAL)
        COMPARE_CASE(NEQ, Condition::NOT_EQUAL)
        COMPARE_CASE(LT, Condition::LESS)
        COMPARE_CASE(LTE, Condition::LESS_EQUAL)
        COMPARE_CASE(GT, Condition::GREATER)
        COMPARE_CASE(GTE, Condition::GREATER_EQUAL)
        case NOT: {
            auto a = stack.pop();
            auto res = newValue();
            res->v = !a->v.toBool();
            stack.push(res);
            break;
        }
        case CMP: {
            auto b = stack.pop();
            auto res = get_if<bool>(&b->v);
            ASSERT(res != nullptr, "CMP: operand is not comparable (bool)");
            if(*res) frame->pc++;
            break;
        }
        case JMP: {
            auto offset = (i16) instruction.operands[0];
            frame->pc += offset - 1;
            break;
        }
        case CALL: {
            auto name = instruction.operands[0];
            this->enterFunction(name);
            break;
        }
        case EXPORT: {
            auto pattern = convert(stack.pop());
            pattern->setVariableName(lookupString(instruction.operands[0]));
            this->m_patterns.push_back(pattern);
            break;
        }
        case RETURN: {
            this->leaveFunction();
            break;
        }
    }
}

void VirtualMachine::accessData(u64 address, void *buffer, size_t size, u64 sectionId, bool write) {
    wolv::util::unused(sectionId);
    if (size == 0 || buffer == nullptr)
        return;
    if (!write) {
        this->m_io.read(address, reinterpret_cast<u8*>(buffer), size);
    } else {
       this->m_io.write(address, reinterpret_cast<u8*>(buffer), size);
    }
}

Value VirtualMachine::compare(const Value& b, const Value& a, Condition condition) {
    auto res = newValue();
    if(UNLIKELY(a == b)) { // same object
        res->v = true;
    } else if(a->v.index() == b->v.index()) { // same type, no conversion needed
        res->v = a->v == b->v;
    } else { // symbolic comparison
        auto &va = a->v;
        auto &vb = b->v;
        switch(va.index()) {
            case 0: // BOOL
            case 1: // U128
            case 2: { // I128
                // do numeric comparison
                u128 ia = va.toInteger();
                u128 ib = vb.toInteger();
                switch (condition) {
                    case Condition::EQUAL:
                        res->v = ia == ib;
                        break;
                    case Condition::NOT_EQUAL:
                        res->v = ia != ib;
                        break;
                    case Condition::GREATER:
                        res->v = ia > ib;
                        break;
                    case Condition::GREATER_EQUAL:
                        res->v = ia >= ib;
                        break;
                    case Condition::LESS:
                        res->v = ia < ib;
                        break;
                    case Condition::LESS_EQUAL:
                        res->v = ia <= ib;
                        break;
                }
            }
        }
    }

    return res;
}
