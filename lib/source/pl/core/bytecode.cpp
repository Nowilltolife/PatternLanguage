#include <pl/core/ast/ast_node_struct.hpp>
#include <pl/core/ast/ast_node_type_decl.hpp>

#include "pl/core/errors/parser_errors.hpp"
#include "pl/core/ast/ast_node_variable_decl.hpp"

using namespace pl;
using namespace pl::core;
using namespace pl::core::ast;
using namespace pl::core::instr;

std::string getTypeName(const std::shared_ptr<ASTNodeTypeDecl> &typeDecl) {
    const auto &type = typeDecl->getType();
    // check what node it is
    if(auto typeRedirect = dynamic_cast<ASTNodeTypeDecl*>(type.get()); typeRedirect != nullptr) {
        return typeRedirect->getName();
    } else if(auto builtinType = dynamic_cast<ASTNodeBuiltinType*>(type.get()); builtinType != nullptr) {
        return Token::getTypeName(builtinType->getType());
    } else {
        err::P0002.throwError("Don't know how to get type name of non-type declaration", {}, 0);
    }
}

void variableDeclaration(const ASTNodeVariableDecl* var, BytecodeEmitter &emitter, bool local) {
    auto &type = var->getType()->getType();
    auto &placementOffset = var->getPlacementOffset();
    const std::string& name = var->getName();
    std::string typeName;
    if(auto builtinType = dynamic_cast<ASTNodeBuiltinType*>(type.get()); builtinType != nullptr) {
        auto bType = builtinType->getType();
        typeName = Token::getTypeName(bType);
        if(local) {
            emitter.read_value((u16) bType);
        } else {
            emitter.read_field(name, typeName, (u16) bType);
        }
    } else if(auto typeDecl = dynamic_cast<ASTNodeTypeDecl*>(type.get()); typeDecl != nullptr) {
        if(auto structType = dynamic_cast<ASTNodeStruct*>(typeDecl->getType().get()); structType != nullptr) {
            typeName = typeDecl->getName();
            emitter.new_struct(typeName);
            emitter.call(instr::ctor_name + typeName);
            if(!local) emitter.store_field(name, typeName, true);
        }
    } else {
        err::P0002.throwError("Don't know how to emit non-builtin type", {}, 0);
    }
    if(local) {
        if (placementOffset != nullptr) {
            emitter.dup(); // copy for export
        }
        emitter.store_local(name, typeName);
        if(placementOffset != nullptr) {
            emitter.export_();
        }
    }
}

void ASTNodeStruct::emit(Bytecode &bytecode, BytecodeEmitter &emitter) {
    for(auto &super : this->m_inheritance) {
        auto typeDecl = dynamic_cast<ASTNodeTypeDecl *>(super.get());
        if(!typeDecl) {
            err::P0002.throwError("Don't know how to inherit from non-type declaration", {}, 0);
        }
        emitter.load_local(instr::this_name);
        emitter.call(instr::ctor_name + typeDecl->getName());
    }
    for (auto &member : this->m_members) {
        if(auto variableDecl = dynamic_cast<ASTNodeVariableDecl*>(member.get()); variableDecl != nullptr) {
            variableDeclaration(variableDecl, emitter, false);
        } else {
            member->emit(bytecode, emitter);
        }
    }
}

void ASTNodeVariableDecl::emit(instr::Bytecode &bytecode, instr::BytecodeEmitter &emitter) {
    hlp::unused(bytecode);
    variableDeclaration(this, emitter, true);
}

void ASTNodeTypeDecl::emit(instr::Bytecode &bytecode, instr::BytecodeEmitter &emitter) {
    hlp::unused(emitter);
    if(this->m_valid) {
        if(auto structType = dynamic_cast<ASTNodeStruct*>(this->m_type.get()); structType != nullptr) {
            if(this->m_completed) {
                auto structEmitter = bytecode.function(instr::ctor_name + this->m_name);
                structType->emit(bytecode, structEmitter);
                structEmitter.return_();
            }
        }
    }
}