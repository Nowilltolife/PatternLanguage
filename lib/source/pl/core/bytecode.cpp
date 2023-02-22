#include <pl/core/ast/ast_node_struct.hpp>
#include <pl/core/ast/ast_node_type_decl.hpp>

#include "pl/core/errors/parser_errors.hpp"
#include "pl/core/ast/ast_node_variable_decl.hpp"

using namespace pl::core;
using namespace pl::core::ast;
using namespace pl::core::instr;

void ASTNodeStruct::emit(Bytecode &bytecode, BytecodeEmitter &emitter) {
    for(auto &super : this->m_inheritance) {
        auto typeDecl = dynamic_cast<ASTNodeTypeDecl *>(super.get());
        if(!typeDecl) {
            err::P0002.throwError("Don't know how to inherit from non-type declaration", {}, 0);
        }
        emitter.load_local(instr::this_name);
        emitter.call("<init>" + typeDecl->getName());
    }
    for (auto &member : this->m_members) {
        // TODO: split ASTNodeVariableDecl into ASTNodePlacedVariableDecl and ASTNodeLocalVariableDecl
        member->emit(bytecode, emitter);
        if(auto variableDecl = dynamic_cast<ASTNodeVariableDecl*>(member.get()); variableDecl != nullptr) {
            emitter.store_field(variableDecl->getName(), true);
        }
    }
}

void ASTNodeVariableDecl::emit(instr::Bytecode &bytecode, instr::BytecodeEmitter &emitter) {
    hlp::unused(bytecode);
    if(this->m_type->isValid()) {
        auto type = this->m_type->getType();
        if(auto builtinType = dynamic_cast<ASTNodeBuiltinType*>(type.get()); builtinType != nullptr) {
            emitter.read_field(getName(), (u16) builtinType->getType());
        } else if(auto typeDecl = dynamic_cast<ASTNodeTypeDecl*>(type.get()); typeDecl != nullptr) {
            if(auto structType = dynamic_cast<ASTNodeStruct*>(typeDecl->getType().get()); structType != nullptr) {
                emitter.new_struct(typeDecl->getName());
                emitter.call(instr::ctor_name + typeDecl->getName());
            }
        } else {
            err::P0002.throwError("Don't know how to emit non-builtin type", {}, 0);
        }
        if(this->m_placementOffset != nullptr) {
            emitter.export_();
        }
    }
}

void ASTNodeTypeDecl::emit(instr::Bytecode &bytecode, instr::BytecodeEmitter &emitter) {
    hlp::unused(emitter);
    if(this->m_valid) {
        if(auto structType = dynamic_cast<ASTNodeStruct*>(this->m_type.get()); structType != nullptr) {
            if(this->m_completed) {
                auto structEmitter = bytecode.function(instr::ctor_name + this->m_name);
                structType->emit(bytecode, structEmitter);
            }
        }
    }
}