#include <pl/core/evaluator.hpp>
#include <pl/core/vm/vm.hpp>

using namespace pl::core;
using namespace pl::core::vm;
using namespace pl::core::instr;
using pl::core::instr::Opcode;

#define FRAME this->m_frame
#define STATIC(x) this->m_staticNames.x
#define LOCAL(x) FRAME->locals[x]
#define STACK FRAME->stack
#define THIS LOCAL(STATIC(thisName))

template <typename T>
auto vpop(T & stack) {
    auto v = std::move(stack.top());
    stack.pop();
    return v;
}

void VirtualMachine::loadBytecode(Bytecode bytecode) {
    this->m_symbolTable = bytecode.getSymbolTable();
    // initalize statics
    this->m_staticNames = {
        .thisName = this->m_symbolTable.newString(this_name),
        .mainName = this->m_symbolTable.newString(main_name),
        .globalName = this->m_symbolTable.newString("main")
    };
}

void VirtualMachine::enterFunction(u32 name) {
    // retrieve from function table
    auto function = this->lookupFunction(name);
    auto actualName = dynamic_cast<StringSymbol* >(this->m_symbolTable.getSymbol(function.name));
    auto prev = this->m_frame;
    auto frame = new Frame();
    frame->pc = 0;
    if(prev != nullptr) {
        if(actualName->value[0] == '<') { // is initializer
            frame->locals[this->m_staticNames.thisName] = vpop(prev->stack);
        }
    }
    // set new frame
    this->m_frame = frame;
    this->m_frames.push(frame);
    this->m_instructions = *function.instructions;
}

void VirtualMachine::leaveFunction() {
    if(this->m_frames.empty()) {
        // shutdown vm
        this->m_running = false;
    } else {
        this->m_frame = vpop(this->m_frames);
    }
}

void VirtualMachine::run() {

}

void VirtualMachine::step() {
    auto instruction = this->m_instructions[FRAME->pc];
    switch (instruction->opcode) {
        case STORE_IN_THIS: {
            auto op = static_cast<IntOpInstruction *>(instruction);
            auto value = vpop(STACK);
            if(THIS->type != ValueType::STRUCTURE) {
                err::E0001.throwError(fmt::format("#{:x} store_in_this failed: this is not a structure", (u8) instruction->opcode));
            }
            THIS->field[op->value].value = value;
            break;
        }
        case LOAD_FROM_THIS: {
            auto op = static_cast<IntOpInstruction *>(instruction);
            if(THIS->type != ValueType::STRUCTURE) {
                err::E0001.throwError(fmt::format("#{:x} load_from_this failed: this is not a structure", (u8) instruction->opcode));
            }
            STACK.push(THIS->field[op->value].value);
            break;
        }
        case STORE_FIELD: {
            auto op = static_cast<IntOpInstruction *>(instruction);
            auto value = vpop(STACK);
            auto structure = vpop(STACK);
            if(structure->type != ValueType::STRUCTURE) {
                err::E0001.throwError(fmt::format("#{:x} store_field failed: TOS is not a structure", (u8) instruction->opcode));
            }
            structure->field[op->value].value = value;
            break;
        }
        case LOAD_FIELD:
            break;
        case STORE_ATTRIBUTE:
            break;
        case STORE_LOCAL:
            break;
        case LOAD_LOCAL:
            break;
        case STORE_GLOBAL:
            break;
        case LOAD_GLOBAL:
            break;
        case NEW_STRUCT:
            break;
        case READ_FIELD: {
            auto op = static_cast<BiIntOpInstruction *>(instruction);

            break;
        }
        case CALL:
            break;
        case EXPORT:
            break;
    }
}