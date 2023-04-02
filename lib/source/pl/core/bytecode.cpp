#include <pl/core/ast/ast_node_struct.hpp>
#include <pl/core/ast/ast_node_type_decl.hpp>
#include <pl/core/ast/ast_node_lvalue_assignment.hpp>
#include <pl/core/ast/ast_node_compound_statement.hpp>
#include <pl/core/ast/ast_node_rvalue.hpp>
#include <pl/core/ast/ast_node_conditional_statement.hpp>
#include <pl/core/ast/ast_node_mathematical_expression.hpp>
#include <pl/core/ast/ast_node_match_statement.hpp>

#include "pl/core/errors/parser_errors.hpp"
#include "pl/core/ast/ast_node_variable_decl.hpp"

using namespace pl;
using namespace pl::core;
using namespace pl::core::ast;
using namespace pl::core::instr;
using TypeId = pl::core::instr::TypeInfo::TypeId;
using Label = pl::core::instr::BytecodeEmitter::Label;

std::string BytecodeEmitter::getTypeName(const std::shared_ptr<ast::ASTNode>& type) const {
    // check what node it is
    if(auto typeNode = dynamic_cast<ASTNodeTypeDecl*>(type.get()); typeNode != nullptr) {
        auto name = typeNode->getName();
        if(name.empty()) {
            return getTypeName(typeNode->getType());
        } else {
            return name;
        }
    } else if(auto builtinNode = dynamic_cast<ASTNodeBuiltinType*>(type.get()); builtinNode != nullptr) {
        return Token::getTypeName(builtinNode->getType());
    }
    err::P0003.throwError("Unknown type node");
}

std::pair<TypeInfo, std::string> BytecodeEmitter::getTypeInfo(const std::unique_ptr<ASTNode>& type, const std::string& name) {
    if(auto builtinType = dynamic_cast<ASTNodeBuiltinType*>(type.get()); builtinType != nullptr) {
        return {{TypeInfo::fromLiteral(builtinType->getType()), getSymbolTable().newString(name)}, name};
    } else if(auto structType = dynamic_cast<ASTNodeStruct*>(type.get()); structType != nullptr) {
        return {{TypeId::Structure, getSymbolTable().newString(name)}, name};
    } else {
        const static std::pair<TypeInfo, std::string> empty = {{(TypeId) 0, 0}, ""};
        return empty;
    }
}

void variableRead(const ASTNodeVariableDecl* var, BytecodeEmitter &emitter, bool local) {
    const auto& type = var->getType();
    //auto &placementOffset = var->getPlacementOffset();
    const std::string& name = var->getName();
    auto [typeInfo, typeName] = emitter.getTypeInfo(type->resolveType(), emitter.getTypeName(type));
    if(typeInfo.name == 0) {
        err::P0002.throwError("Can't declare variable of this type", {}, 0);
    }
    if(local) {
        emitter.read_value(typeInfo);
        emitter.local(name, typeName);
        emitter.dup();
        emitter.store_local(name, typeName);
        emitter.export_(name);
    } else {
        emitter.read_field(name, typeInfo);
    }
}

void ASTNodeVariableDecl::emit(instr::Bytecode &bytecode, instr::BytecodeEmitter &emitter) {
    wolv::util::unused(bytecode);
    if(emitter.flags.ctor) {
        variableRead(this, emitter, false);
        return;
    } else if(m_placementOffset != nullptr) {
        variableRead(this, emitter, true);
    } else {
        emitter.local(getName(), emitter.getTypeName(getType()));
    }
}

void ASTNodeTypeDecl::emit(instr::Bytecode &bytecode, instr::BytecodeEmitter &emitter) {
    wolv::util::unused(emitter);
    if(this->m_valid) {
        if(auto structType = dynamic_cast<ASTNodeStruct*>(this->m_type.get()); structType != nullptr) {
            if(this->m_completed) {
                auto structEmitter = bytecode.function(instr::ctor_name + this->m_name);
                structEmitter.flags.ctor = true;
                structType->emit(bytecode, structEmitter);
                structEmitter.return_();
            }
        } else if(auto typeNode = dynamic_cast<ASTNodeTypeDecl*>(this->m_type.get()); typeNode != nullptr) {
            if(this->m_completed) {
                typeNode->emit(bytecode, emitter);
                auto [typeInfo, _] = emitter.getTypeInfo(typeNode->resolveType(), typeNode->getName());
                if(TypeInfo::isComplex(typeInfo.id)) {
                    auto structEmitter = bytecode.function(instr::ctor_name + this->m_name);
                    structEmitter.flags.ctor = true;
                    structEmitter.call(instr::ctor_name + typeNode->getName());
                    structEmitter.return_();
                }
            }
        }
    }
}

void ASTNodeStruct::emit(instr::Bytecode &bytecode, instr::BytecodeEmitter &emitter) {
    for(auto &super : this->m_inheritance) {
        auto typeDecl = dynamic_cast<ASTNodeTypeDecl *>(super.get());
        if(!typeDecl) {
            err::P0002.throwError("Don't know how to inherit from non-type declaration", {}, 0);
        }
        emitter.load_local(instr::this_name);
        emitter.call(instr::ctor_name + typeDecl->getName());
    }
    for (auto &member : this->m_members) {
        member->emit(bytecode, emitter);
    }
}

void BytecodeEmitter::store_value(const std::string &name, std::string &typeName) {
    if(!flags.ctor) {
        local(name, typeName);
        dup();
        store_local(name, typeName);
        export_(name);
    } else {
        store_field(name, typeName, true);
    }
}