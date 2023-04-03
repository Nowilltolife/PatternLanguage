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
#include <pl/patterns/pattern_array_static.hpp>
#include <pl/patterns/pattern_array_dynamic.hpp>
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
    case op: {                      \
        auto b = stack.pop();       \
        auto a = stack.pop();       \
        stack.push(compareValues(a, b, operation)); \
        break; \
    }

using namespace pl;
using namespace pl::core;
using namespace pl::core;
using namespace pl::core::instr;
using namespace pl::ptrn;
using pl::core::instr::TypeInfo;
using pl::core::instr::Opcode, pl::core::instr::Operand, pl::core::instr::SymbolId;
using std::get_if;

using TypeId = pl::core::instr::TypeInfo::TypeId;

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
    constexpr static std::array Palette = {0x70B4771F, 0x700E7FFF, 0x702CA02C, 0x702827D6, 0x70BD6794, 0x704B568C,
                                           0x70C277E3, 0x7022BDBC, 0x70CFBE17};

    auto index = colorIndex;
    colorIndex = (colorIndex + 1) % Palette.size();

    return Palette[index];
}

void VirtualMachine::initialize() {
    this->m_addressValue = newValue();
    this->m_addressValue->setValue((u128) 0);
    this->m_address.set(std::get_if<u128>(&this->m_addressValue->m_value));
}

bool VirtualMachine::readValue(Operand type, TypeId id, bool next) {
    if(m_frame->escapeNow) {
        m_frame->escapeNow = false;
        return true;
    }
    auto value = newValue();
#ifdef DEBUG
    value->type = id;
#endif
    if(TypeInfo::isBuiltin(id)) {
        value->size = TypeInfo::getTypeSize(id);
        value->address = this->m_address;
        value->section = 0;
        if(TypeInfo::isUnsigned(id)) {
            u128 v = 0;
            readData(value->address, &v, value->size, 0);
            value->setValue(v);
        } else if(TypeInfo::isSigned(id)) {
            i128 v = 0;
            readData(value->address, &v, value->size, 0);
            value->setValue(v);
        } else if(id == TypeInfo::Float) {
            float v = 0;
            readData(value->address, &v, value->size, 0);
            value->setValue(v);
        }
        this->m_address += value->size;
        m_frame->stack.push(value);
        return true;
    } else if(TypeInfo::isComplex(id)) {
        switch (id) {
            case TypeInfo::Structure: {
                Struct structure = {};
                structure.typeName = type;
                structure.fields = {};
                structure.address = this->m_address;
                value->setValue(structure);
                value->address = this->m_address;
                m_frame->stack.push(value);
                auto constructor = lookupConstructor(type);
                ASSERT(constructor.name != 0, "constructor not found");
                if(next) {
                    this->m_frame->escapeNow = true;
                    this->m_frame->pc--;
                }
                enterFunction(constructor.name, true); // constructor MUST push new struct onto stack
                // also tell the vm to return back one instruction after enter, to allow function be executed
                return false;
            }
            default: {
                err::E0001.throwError(fmt::format("#{:x} read_value failed: invalid type", m_frame->pc));
            }
        }
    } else {
        err::E0001.throwError(fmt::format("#{:x} read_value failed: invalid type", m_frame->pc));
    }
}

std::unique_ptr<Pattern> VirtualMachine::convert(const Value& value) {
    auto a = std::visit(overloaded {
        [&](bool b) {
            wolv::util::unused(b);
            return (std::unique_ptr<Pattern>) std::make_unique<PatternBoolean>(nullptr, value->address);
        },
        [&](double d) {
            wolv::util::unused(d);
            return (std::unique_ptr<Pattern>) std::make_unique<PatternFloat>(nullptr, value->address, value->size);
        },
        [&](u128 v) {
            wolv::util::unused(v);
            return (std::unique_ptr<Pattern>) std::make_unique<PatternUnsigned>(nullptr, value->address, value->size);
        },
        [&](i128 v) {
            wolv::util::unused(v);
            return (std::unique_ptr<Pattern>) std::make_unique<PatternSigned>(nullptr, value->address, value->size);
        },
        [&](const Struct& strct) {
            std::vector<std::shared_ptr<Pattern>> members;
            u64 size = 0;
            for (const auto &[name, field]: strct.fields) {
                auto fieldPattern = std::shared_ptr<Pattern>(convert(field.value).release());
                fieldPattern->setVariableName(lookupString(field.name));
                fieldPattern->setTypeName(lookupString(field.typeName));
                members.push_back(fieldPattern);
                size += fieldPattern->getSize();
            }
            auto pattern = std::make_unique<PatternStruct>(nullptr, value->address, size);
            pattern->setTypeName(lookupString(strct.typeName));
            pattern->setVariableName(lookupString(strct.name));
            pattern->setMembers(members);
            return (std::unique_ptr<Pattern>) std::move(pattern);
        },
        [&](Field* field) {
            auto fieldPattern = convert(field->value);
            fieldPattern->setVariableName(lookupString(field->name));
            fieldPattern->setTypeName(lookupString(field->typeName));
            return (std::unique_ptr<Pattern>) std::move(fieldPattern);
        },
        [&](const StaticArray& array) {
            auto pattern = std::make_unique<PatternArrayStatic>(nullptr, value->address, array.size * array.templateValue->size);
            auto templatePattern = convert(array.templateValue);
            templatePattern->setTypeName(lookupString(array.type));
            pattern->setEntries(std::move(templatePattern), array.size);
            return (std::unique_ptr<Pattern>) std::move(pattern);
        },
        [&](const DynamicArray& array) {
            std::vector<std::shared_ptr<Pattern>> entries;
            u16 size = 0;
            for (const auto& entry: array.values) {
                auto newEntry = std::shared_ptr<Pattern>(convert(entry).release());
                newEntry->setVariableName(fmt::format("[{}]", entries.size()));
                newEntry->setTypeName(lookupString(array.type));
                entries.push_back(newEntry);
                size += newEntry->getSize();
            }
            auto pattern = std::make_unique<PatternArrayDynamic>(nullptr, value->address, size);
            pattern->setEntries(std::move(entries));
            return (std::unique_ptr<Pattern>) std::move(pattern);
        },
        [&](const Value& value) {
            return (std::unique_ptr<Pattern>) convert(value);
        }
    }, value->m_value);
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
        .globalName = this->m_symbolTable.newString("main"),
        .addressName = this->m_symbolTable.newString(address_name),
    };
    this->m_functions = bytecode.getFunctions();
    colorIndex = 0;
}

void VirtualMachine::enterFunction(SymbolId name, bool ctor) {
    // retrieve from function table
    auto function = this->lookupFunction(name);
    ASSERT(function.name != 0, fmt::format("Function with name {} not found", name).c_str());
    this->m_function = function;
    auto prev = this->m_frame;
    auto frame = new Frame();
    frame->pc = 0;
    frame->m_instructions = function.instructions;
    if(ctor) { // is initializer
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
    this->m_address = static_cast<u128>(0);
    this->m_patterns.clear();
}

void VirtualMachine::step() {
    auto frame = this->m_frame;
    auto &stack = frame->stack;
    auto &locals = frame->locals;
    const auto &instruction = (*frame->m_instructions)[frame->pc++];
    const auto &operands = instruction.operands;
    switch (instruction.opcode) {
        case STORE_IN_THIS: {
            auto &name = operands[0];
            auto &typeName = operands[1];
            auto value = stack.pop();
            auto v = locals[m_staticNames.thisName]->toStruct();
            ASSERT(v != nullptr, "'this' is not a structure");
            auto &f = (*v).fields[name];
            f.value = value;
            f.name = name;
            f.typeName = typeName;
            break;
        }
        case LOAD_FROM_THIS: {
            auto &index = operands[0];
            auto v = locals[m_staticNames.thisName]->toStruct();
            ASSERT(v != nullptr, "'this' is not a structure");
            stack.push((*v).fields[index].value);
            break;
        }
        case STORE_FIELD: {
            auto &name = operands[0];
            auto value = stack.pop();
            auto structure = stack.pop();
            auto v = structure->toStruct();
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
            auto &index = operands[0];
            if(index == m_staticNames.addressName) {
                m_addressValue->setValue(stack.pop()->toUnsigned());
                break;
            }
            //auto &typeName = instruction.operands[1];
            locals[index] = stack.pop();
            break;
        }
        case LOAD_LOCAL: {
            auto &index = operands[0];
            if(index == m_staticNames.addressName) {
                stack.push(m_addressValue);
                break;
            }
            if(!locals.contains(index))
                err::E0003.throwError(fmt::format("No variable named '{}' found.", lookupString(index)), {}, 0);
            stack.push(locals[index]);
            break;
        }
        case LOAD_SYMBOL: {
            auto &index = operands[0];
            auto symbol = this->m_symbolTable.getSymbol(index);
            auto value = newValue();
            switch (symbol->type) {
                case SymbolType::STRING: {
                    // TODO: strings
                    //value->v = dynamic_cast<StringSymbol*>(symbol)->value;
                    break;
                }
                case SymbolType::UNSIGNED_INTEGER: {
                    value->setValue((u128) dynamic_cast<UISymbol*>(symbol)->value);
                    break;
                }
                case SymbolType::SIGNED_INTEGER: {
                    value->setValue((i128) dynamic_cast<SISymbol*>(symbol)->value);
                    break;
                }
            }
            stack.push(value);
            break;
        }
        case NEW_STRUCT: {
            auto &name = operands[0];
            auto value = newValue();
            Struct structure = {};
            structure.typeName = name;
            structure.fields = {};
            value->setValue(structure);
            value->address = this->m_address;
            stack.push(value);
            break;
        }
        case READ_STATIC_ARRAY: {
            auto cond = stack.pop();
            auto &state = frame->arrayState;
            if(state.size == 0) {
                state.templateValue = stack.pop();
                state.size = state.templateValue->size;
            }
            if(cond->toBoolean()) {
                state.index++;
                this->m_address += state.size;
                frame->pc = operands[0];
            } else {
                auto array = newValue();
                StaticArray staticArray = {};
                staticArray.templateValue = state.templateValue;
                staticArray.type = operands[1];
                staticArray.size = state.index * state.size;
                array->setValue(staticArray);
                array->size = staticArray.size;
                array->address = this->m_address;
                stack.push(array);
                state = {};
            }
            break;
        }
        case READ_DYNAMIC_ARRAY: {
            if(frame->escapeNow) stack.swap();
            auto cond = stack.pop();
            auto &state = frame->arrayState;
            if(state.index == 0) {
                state.array = {};
            }
            if(cond->toBoolean()) {
                if(!readValue(operands[1], (TypeId) operands[2], true)) {
                    stack.push(cond);
                    return;
                }
                state.index++;
                auto value = stack.pop();
                state.array.values.push_back(value);
                state.size += value->size;
                frame->pc = operands[0];
            } else {
                auto array = newValue();
                state.array.type = operands[1];
                array->setValue(state.array);
                array->address = this->m_address;
                array->size = state.size;
                stack.push(array);
                state = {};
            }
            break;
        }
        case READ_DYNAMIC_ARRAY_WITH_SIZE: {
            auto &state = frame->arrayState;
            if(state.size == 0) {
                auto size = stack.pop()->toUnsigned();
                state = {0, size, operands[0], (TypeId) operands[1], {{}, operands[0]}, {}};
                frame->pc--;
            } else {
                if(!readValue(state.type, state.id, true)) return;
                auto value = stack.pop();
                state.array.values.push_back(value);
                state.index++;
                if(state.index == state.size) {
                    auto array = newValue();
                    array->setValue(state.array);
                    stack.push(array);
                    state = {};
                } else {
                    frame->pc--;
                }
            }
            break;
        }
        case READ_STATIC_ARRAY_WITH_SIZE: {
            auto size = stack.pop()->toSigned();
            auto value = stack.pop();
            auto array = newValue();
            StaticArray a = {value, operands[0], static_cast<u16>(size)};
            array->setValue(a);
            array->address = this->m_address - static_cast<u128>(value->size);
            array->size = value->size * size;
            stack.push(array);
            break;
        }
        case READ_VALUE: {
            readValue(operands[0], (TypeId) operands[1]);
            break;
        }
        case READ_FIELD: {
            auto &name = operands[0];
            auto &type = operands[1];
            auto id = (TypeId) operands[2];
            auto v = locals[m_staticNames.thisName]->toStruct();
            ASSERT(v != nullptr, "'this' is not a structure");
            if(!readValue(type, id, true)) return;
            auto &f = (*v).fields[name];
            f.value = stack.pop();
            f.name = name;
            f.typeName = type;
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
            res->setValue(!a->toBoolean());
            stack.push(res);
            break;
        }
        case CMP: {
            if(stack.pop()->toBoolean()) frame->pc++;
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
            this->m_patterns.push_back(std::move(pattern));
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

template<typename A, typename B>
static bool compare(const A &a, const B &b, Condition condition) {
    switch (condition) {
        case Condition::EQUAL:
            return a == b;
        case Condition::NOT_EQUAL:
            return a != b;
        case Condition::LESS:
            return a < b;
        case Condition::LESS_EQUAL:
            return a <= b;
        case Condition::GREATER:
            return a > b;
        case Condition::GREATER_EQUAL:
            return a >= b;
    }
}

template <typename T>
concept Integer = std::is_integral_v<T>;
template <Integer A, Integer B>
bool static signedCompare(const A &a, const B &b, Condition condition) {
    switch (condition) {
        case Condition::EQUAL:
            return std::cmp_equal(a, b);
        case Condition::NOT_EQUAL:
            return std::cmp_not_equal(a, b);
        case Condition::LESS:
            return std::cmp_less(a, b);
        case Condition::LESS_EQUAL:
            return std::cmp_less_equal(a, b);
        case Condition::GREATER:
            return std::cmp_greater(a, b);
        case Condition::GREATER_EQUAL:
            return std::cmp_greater_equal(a, b);
    }
    return false;
}

Value VirtualMachine::compareValues(const Value& a, const Value& b, Condition condition) {
    auto res = newValue();
    if(UNLIKELY(a == b)) { // same object
        res->setValue(true);
    } else if(a->m_value.index() == b->m_value.index()) { // same type, no conversion needed
        switch (condition) {
            case Condition::EQUAL:
                res->setValue(a->m_value == b->m_value);
                break;
            case Condition::NOT_EQUAL:
                res->setValue(a->m_value != b->m_value);
                break;
            case Condition::LESS:
                res->setValue(a->m_value < b->m_value);
                break;
            case Condition::LESS_EQUAL:
                res->setValue(a->m_value <= b->m_value);
                break;
            case Condition::GREATER:
                res->setValue(a->m_value > b->m_value);
                break;
            case Condition::GREATER_EQUAL:
                res->setValue(a->m_value >= b->m_value);
                break;
        }
    } else { // symbolic comparison
        bool result = std::visit(wolv::util::overloaded {
            [&condition](u128 a, i128 b) {
                return signedCompare(a, b, condition);
            },
            [&condition](i128 a, u128 b) {
                return signedCompare(a, b, condition);
            },
            [](auto&, auto&) {
                return false;
            }
        }, a->m_value, b->m_value);
        res->setValue(result);
        return res;
    }

    return res;
}

Value pl::core::newValue() {
    return std::make_shared<impl::ValueImpl>();
}
