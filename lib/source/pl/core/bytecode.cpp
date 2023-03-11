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
using Label = pl::core::instr::BytecodeEmitter::Label;

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

void variableRead(const ASTNodeVariableDecl* var, BytecodeEmitter &emitter, bool local) {
    auto &type = var->getType()->getType();
    //auto &placementOffset = var->getPlacementOffset();
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
        emitter.local(name, typeName);
        emitter.dup();
        emitter.store_local(name, typeName);
        emitter.export_(name);
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
        member->emit(bytecode, emitter);
    }
}

void ASTNodeConditionalStatement::emit(Bytecode &bytecode, BytecodeEmitter &emitter) {
    m_condition->emit(bytecode, emitter);
    emitter.cmp(); // if true, skip next instruction
    auto elseLabel = Label();
    emitter.jmp(elseLabel); // jump to else block
    for (const auto &item: m_trueBody)
        item->emit(bytecode, emitter);
    emitter.place_label(elseLabel); // else block
    for (const auto &item: m_falseBody)
        item->emit(bytecode, emitter);
    emitter.resolve_label(elseLabel);
}

void ASTNodeVariableDecl::emit(instr::Bytecode &bytecode, instr::BytecodeEmitter &emitter) {
    hlp::unused(bytecode);
    if(emitter.flags.ctor) {
        variableRead(this, emitter, false);
        return;
    } else if(m_placementOffset != nullptr) {
        variableRead(this, emitter, true);
    } else {
        auto &type =getType()->getType();
        const std::string& name = getName();
        std::string typeName;

        if(auto builtinType = dynamic_cast<ASTNodeBuiltinType*>(type.get()); builtinType != nullptr) {
            typeName = Token::getTypeName(builtinType->getType());
        } else if(auto typeDecl = dynamic_cast<ASTNodeTypeDecl*>(type.get()); typeDecl != nullptr) {
            if(auto structType = dynamic_cast<ASTNodeStruct*>(typeDecl->getType().get()); structType != nullptr) {
                typeName = typeDecl->getName();
            }
        } else {
            err::P0002.throwError("Don't know how to emit non-builtin type", {}, 0);
        }
        emitter.local(name, typeName);
    }
}

void ASTNodeCompoundStatement::emit(instr::Bytecode &bytecode, instr::BytecodeEmitter &emitter) {
    for(auto &statement : this->m_statements) {
        statement->emit(bytecode, emitter);
    }
}

void ASTNodeMatchStatement::emit(instr::Bytecode &bytecode, instr::BytecodeEmitter &emitter) {
    auto endLabel = Label();
    for (const auto &item: this->m_cases){
        item.condition->emit(bytecode, emitter);
        emitter.cmp();
        auto elseLabel = Label();
        emitter.jmp(elseLabel);
        for (const auto &statement: item.body)
            statement->emit(bytecode, emitter);
        emitter.jmp(endLabel);
        emitter.place_label(elseLabel);
        emitter.resolve_label(elseLabel);
    }
    if(this->m_defaultCase) {
        for (const auto &statement : this->m_defaultCase->body)
            statement->emit(bytecode, emitter);
    }
    emitter.place_label(endLabel);
}

void ASTNodeMathematicalExpression::emit(instr::Bytecode &bytecode, instr::BytecodeEmitter &emitter) {
    hlp::unused(bytecode);
    m_left->emit(bytecode, emitter);
    m_right->emit(bytecode, emitter);
    switch (m_operator) {
        case Token::Operator::BoolEqual:
            emitter.eq();
            break;
        case Token::Operator::BoolNotEqual:
            emitter.neq();
            break;
        case Token::Operator::BoolLessThan:
            emitter.lt();
            break;
        case Token::Operator::BoolLessThanOrEqual:
            emitter.lte();
            break;
        case Token::Operator::BoolGreaterThan:
            emitter.gt();
            break;
        case Token::Operator::BoolGreaterThanOrEqual:
            emitter.gte();
            break;
        case Token::Operator::BoolAnd:
            emitter.and_();
            break;
        case Token::Operator::BoolOr:
            emitter.or_();
            break;
        case Token::Operator::BoolNot:
            emitter.not_();
            break;
        default:
            err::P0002.throwError("Don't know how to emit operator", {}, 0);
    }
}

void ASTNodeRValue::emit(instr::Bytecode &bytecode, instr::BytecodeEmitter &emitter) {
    hlp::unused(bytecode);
    if(m_path.size() == 1) {
        auto name = std::get<std::string>(m_path[0]);
        emitter.flags.ctor ? emitter.load_field(name, true) : emitter.load_local(name);
    }
}

void ASTNodeLiteral::emit(instr::Bytecode &bytecode, instr::BytecodeEmitter &emitter) {
    hlp::unused(bytecode);
    if(Token::isUnsigned(m_literal.getType())) {
        u16 symbol = bytecode.getSymbolTable().newUnsignedInteger(m_literal.toUnsigned());
        emitter.load_symbol(symbol);
    } else if(Token::isSigned(m_literal.getType())) {
        u16 symbol = bytecode.getSymbolTable().newSignedInteger((i64) m_literal.toSigned());
        emitter.load_symbol(symbol);
    } else {
        err::P0002.throwError("Don't know how to emit literal", {}, 0);
    }
}

void ASTNodeLValueAssignment::emit(instr::Bytecode &bytecode, instr::BytecodeEmitter &emitter) {
    hlp::unused(bytecode);
    m_rvalue->emit(bytecode, emitter);
    emitter.store_local(m_lvalueName, emitter.getLocalType(m_lvalueName));
}

void ASTNodeTypeDecl::emit(instr::Bytecode &bytecode, instr::BytecodeEmitter &emitter) {
    hlp::unused(emitter);
    if(this->m_valid) {
        if(auto structType = dynamic_cast<ASTNodeStruct*>(this->m_type.get()); structType != nullptr) {
            if(this->m_completed) {
                auto structEmitter = bytecode.function(instr::ctor_name + this->m_name);
                structEmitter.flags.ctor = true;
                structType->emit(bytecode, structEmitter);
                structEmitter.return_();
            }
        }
    }
}