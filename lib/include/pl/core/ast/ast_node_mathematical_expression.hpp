#pragma once

#include <pl/core/ast/ast_node.hpp>
#include <pl/core/ast/ast_node_literal.hpp>
#include <pl/core/errors/parser_errors.hpp>
#include <pl/helpers/concepts.hpp>

#include <pl/patterns/pattern_enum.hpp>

namespace pl::core::ast {

    class ASTNodeMathematicalExpression : public ASTNode {

    public:
        ASTNodeMathematicalExpression(std::unique_ptr<ASTNode> &&left, std::unique_ptr<ASTNode> &&right, Token::Operator op)
            : ASTNode(), m_left(std::move(left)), m_right(std::move(right)), m_operator(op) { }

        ASTNodeMathematicalExpression(const ASTNodeMathematicalExpression &other) : ASTNode(other) {
            this->m_operator = other.m_operator;
            this->m_left     = other.m_left->clone();
            this->m_right    = other.m_right->clone();
        }

        [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
            return std::unique_ptr<ASTNode>(new ASTNodeMathematicalExpression(*this));
        }

        void emit(instr::Bytecode &bytecode, instr::BytecodeEmitter &emitter) override{
            hlp::unused(bytecode);
            switch (m_operator) {
                case Token::Operator::BoolEqual:
                    m_left->emit(bytecode, emitter);
                    m_right->emit(bytecode, emitter);
                    emitter.eq();
                    break;
                case Token::Operator::BoolNotEqual:
                    m_left->emit(bytecode, emitter);
                    m_right->emit(bytecode, emitter);
                    emitter.neq();
                    break;
                case Token::Operator::BoolLessThan:
                    m_left->emit(bytecode, emitter);
                    m_right->emit(bytecode, emitter);
                    emitter.lt();
                    break;
                case Token::Operator::BoolLessThanOrEqual:
                    m_left->emit(bytecode, emitter);
                    m_right->emit(bytecode, emitter);
                    emitter.lte();
                    break;
                case Token::Operator::BoolGreaterThan:
                    m_left->emit(bytecode, emitter);
                    m_right->emit(bytecode, emitter);
                    emitter.gt();
                    break;
                case Token::Operator::BoolGreaterThanOrEqual:
                    m_left->emit(bytecode, emitter);
                    m_right->emit(bytecode, emitter);
                    emitter.gte();
                    break;
                case Token::Operator::BoolAnd: {
                    m_left->emit(bytecode, emitter);
                    emitter.dup(); // duplicate left value
                    emitter.cmp(); // if true, skip next instruction
                    auto elseLabel = emitter.label();
                    emitter.jmp(elseLabel);
                    // else pop left value
                    emitter.pop();
                    m_right->emit(bytecode, emitter);
                    emitter.place_label(elseLabel);
                    emitter.resolve_label(elseLabel);
                    break;
                }
                case Token::Operator::BoolOr: {
                    m_left->emit(bytecode, emitter);
                    emitter.dup(); // duplicate left value
                    emitter.not_(); // inverse value
                    emitter.cmp(); // if true, skip next instruction
                    auto elseLabel = emitter.label();
                    emitter.jmp(elseLabel);
                    // else pop left value
                    emitter.pop();
                    m_right->emit(bytecode, emitter);
                    emitter.place_label(elseLabel);
                    emitter.resolve_label(elseLabel);
                    break;
                }
                case Token::Operator::BoolNot:
                    m_left->emit(bytecode, emitter);
                    emitter.not_();
                    break;
                default:
                    err::P0002.throwError("Don't know how to emit operator", {}, 0);
            }
        }

        [[nodiscard]] const std::unique_ptr<ASTNode> &getLeftOperand() const { return this->m_left; }
        [[nodiscard]] const std::unique_ptr<ASTNode> &getRightOperand() const { return this->m_right; }
        [[nodiscard]] Token::Operator getOperator() const { return this->m_operator; }

    private:
        std::unique_ptr<ASTNode> m_left, m_right;
        Token::Operator m_operator;
    };

#undef FLOAT_BIT_OPERATION

}