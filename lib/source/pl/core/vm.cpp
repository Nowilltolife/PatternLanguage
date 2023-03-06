#include <pl/core/evaluator.hpp>
#include <pl/core/vm/vm.hpp>
#include <pl/patterns/pattern.hpp>
#include <pl/patterns/pattern_unsigned.hpp>
#include <pl/patterns/pattern_signed.hpp>
#include <pl/patterns/pattern_struct.hpp>
#include <pl/patterns/pattern_boolean.hpp>
#include <pl/patterns/pattern_float.hpp>
#include <variant>
#include <pl/helpers/utils.hpp>

using namespace pl;
using namespace pl::core;
using namespace pl::core::vm;
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

template <typename T>
auto vpop(std::stack<T>& stack) {
    if(stack.empty()) {
        err::E0001.throwError("Stack underflow");
    }
    auto v = std::move(stack.top());
    stack.pop();
    return v;
}

u32 colorIndex = 0;

u32 getNextPalletColor() {
    constexpr static std::array Palette = { 0x70B4771F, 0x700E7FFF, 0x702CA02C, 0x702827D6, 0x70BD6794, 0x704B568C, 0x70C277E3, 0x7022BDBC, 0x70CFBE17 };

    auto index = colorIndex;
    colorIndex = (colorIndex + 1) % Palette.size();

    return Palette[index];
}

Value* VirtualMachine::readStaticValue(u16 type) {
    auto valueType = (Token::ValueType) type;
    auto value = new Value();
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

std::shared_ptr<Pattern> VirtualMachine::convert(Value* value) {
    auto a = std::visit(overloaded {
        [&](bool b) {
            hlp::unused(b);
            return (std::shared_ptr<Pattern>) std::make_shared<PatternBoolean>(nullptr, value->address);
        },
        [&](double d) {
            hlp::unused(d);
            return (std::shared_ptr<Pattern>) std::make_shared<PatternFloat>(nullptr, value->address, value->size);
        },
        [&](u128 v) {
            auto pattern = std::make_shared<PatternUnsigned>(nullptr, value->address, value->size);
            pattern->setData(v);
            return (std::shared_ptr<Pattern>) pattern;
        },
        [&](i128 v) {
            hlp::unused(v);
            return (std::shared_ptr<Pattern>) std::make_shared<PatternSigned>(nullptr, value->address, value->size);
        },
        [&](Struct* strct) {
            std::vector<std::shared_ptr<Pattern>> members;
            u64 size = 0;
            for (const auto &[name, field]: strct->fields) {
                auto fieldPattern = convert(field.value);
                fieldPattern->setVariableName(lookupString(field.name));
                fieldPattern->setTypeName(lookupString(field.typeName));
                members.push_back(fieldPattern);
                size += fieldPattern->getSize();
            }
            auto pattern = std::make_shared<PatternStruct>(nullptr, value->address, size);
            pattern->setTypeName(lookupString(strct->typeName));
            pattern->setVariableName(lookupString(strct->name));
            pattern->setMembers(members);
            return (std::shared_ptr<Pattern>) pattern;
        },
        [&](Field* field) {
            auto fieldPattern = convert(field->value);
            fieldPattern->setVariableName(lookupString(field->name));
            fieldPattern->setTypeName(lookupString(field->typeName));
            return (std::shared_ptr<Pattern>) fieldPattern;
        },
        [&](Value* value) {
            return (std::shared_ptr<Pattern>) convert(value);
        }
    }, value->v);
    a->setColor(getNextPalletColor());
    return a;
}

void VirtualMachine::loadBytecode(Bytecode bytecode) {
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
    auto actualName = dynamic_cast<StringSymbol* >(this->m_symbolTable.getSymbol(function.name));
    auto prev = this->m_frame;
    auto frame = new Frame();
    frame->pc = 0;
    frame->m_instructions = function.instructions;
    if(actualName->value[0] == '<') { // is initializer
        if (prev != nullptr) {
            if (prev->stack.empty())
                err::E0001.throwError(fmt::format("Initializer '{}' called without a structure", actualName->value),
                                      "This is a vm bug");
            const auto &v = vpop(prev->stack);
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
            this->result = vpop(stack);
    } else {
        auto prev = vpop(this->m_frames);
        if(!stack.empty())
            prev->stack.push(vpop(stack)); // return value
        this->m_frame = prev; // switch control over
    }
}

Value* VirtualMachine::executeFunction(const std::string& function) {
    auto name = this->m_symbolTable.newString(function);
    auto fn = this->lookupFunction(name);
    if(fn.name == 0) {
        err::E0001.throwError(fmt::format("Function '{}' not found", function));
    }
    this->enterFunction(this->m_symbolTable.newString(function));
    this->run();
    this->leaveFunction();
    return this->result;
}

void VirtualMachine::run() {
    this->m_running = true;
    while(this->m_running) {
        this->step();
    }
}

void VirtualMachine::step() {
    auto frame = this->m_frame;
    auto &stack = frame->stack;
    auto &locals = frame->locals;
    auto &instruction = (*frame->m_instructions)[frame->pc];
    switch (instruction.opcode) {
        case STORE_IN_THIS: {
            auto &name = instruction.operands[0];
            auto &typeName = instruction.operands[1];
            auto value = vpop(stack);
            auto v = get_if<Struct*>(&locals[m_staticNames.thisName]->v);
            if(v == nullptr) {
                err::E0001.throwError(fmt::format("#{:x} store_in_this failed: this is not a structure", frame->pc));
            }
            auto &f = (*v)->fields[name];
            f.value = value;
            f.name = name;
            f.typeName = typeName;
            break;
        }
        case LOAD_FROM_THIS: {
            auto &index = instruction.operands[0];
            auto v = get_if<Struct*>(&locals[m_staticNames.thisName]->v);
            if(v == nullptr) {
                err::E0001.throwError(fmt::format("#{:x} load_from_this failed: this is not a structure", frame->pc));
            }
            STACK.push((*v)->fields[index].value);
            break;
        }
        case STORE_FIELD: {
            auto &name = instruction.operands[0];
            auto value = vpop(stack);
            auto structure = vpop(stack);
            auto v = get_if<Struct*>(&structure->v);
            if(v == nullptr) {
                err::E0001.throwError(fmt::format("#{:x} store_field failed: structure is not a structure", frame->pc));
            }
            auto &f = (*v)->fields[name];
            f.value = value;
            f.name = name;
            break;
        }
        case DUP: {
            stack.push(stack.top());
            break;
        }
        case LOAD_FIELD:
            break;
        case STORE_ATTRIBUTE:
            break;
        case STORE_LOCAL: {
            auto &index = instruction.operands[0];
            auto &typeName = instruction.operands[1];
            auto value = vpop(stack);
            locals[index] = value;
            // check for values that are subtype of Object
            auto structure = get_if<Struct*>(&value->v);
            if(structure) {
                (*structure)->name = index;
                (*structure)->typeName = typeName;
            }
            break;
        }
        case LOAD_LOCAL: {
            auto &index = instruction.operands[0];
            if(!locals.contains(index))
                err::E0001.throwError(fmt::format("#{:x} load_local failed: local {} not found", frame->pc, index));
            stack.push(locals[index]);
            break;
        }
        case NEW_STRUCT: {
            auto &name = instruction.operands[0];
            auto value = new Value();
            auto structure = new Struct();
            structure->typeName = name;
            structure->fields = {};
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
            auto v = get_if<Struct*>(&locals[m_staticNames.thisName]->v);
            if(v == nullptr) {
                err::E0001.throwError(fmt::format("#{:x} store_in_this failed: this is not a structure", frame->pc));
            }
            auto value = readStaticValue(instruction.operands[2]);
            this->m_address += value->size;
            auto &f = (*v)->fields[name];
            f.value = value;
            f.name = name;
            f.typeName = typeName;
            break;
        }
        case CALL: {
            auto name = instruction.operands[0];
            this->enterFunction(name);
            break;
        }
        case EXPORT: {
            this->m_patterns.push_back(convert(vpop(STACK)));
            break;
        }
        case RETURN: {
            this->leaveFunction();
            break;
        }
    }
}

void VirtualMachine::accessData(u64 address, void *buffer, size_t size, u64 sectionId, bool write) {
    hlp::unused(sectionId);
    if (size == 0 || buffer == nullptr)
        return;
    if (!write) {
        this->m_io.read(address, reinterpret_cast<u8*>(buffer), size);
    } else {
       this->m_io.write(address, reinterpret_cast<u8*>(buffer), size);
    }
}