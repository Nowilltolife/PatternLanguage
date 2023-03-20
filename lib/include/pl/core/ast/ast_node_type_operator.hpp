#pragma once

#include <pl/core/ast/ast_node.hpp>

namespace pl::core::ast {

    class ASTNodeTypeOperator : public ASTNode {
    public:
        ASTNodeTypeOperator(Token::Operator op, std::unique_ptr<ASTNode> &&expression) : m_op(op), m_expression(std::move(expression)) { }
        explicit ASTNodeTypeOperator(Token::Operator op) : m_op(op), m_providerOperation(true) { }

        ASTNodeTypeOperator(const ASTNodeTypeOperator &other) : ASTNode(other) {
            this->m_op = other.m_op;
            this->m_providerOperation = other.m_providerOperation;

            if (other.m_expression != nullptr)
                this->m_expression = other.m_expression->clone();
        }

        [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
            return std::unique_ptr<ASTNode>(new ASTNodeTypeOperator(*this));
        }

        [[nodiscard]] Token::Operator getOperator() const {
            return this->m_op;
        }

        [[nodiscard]] const std::unique_ptr<ASTNode> &getExpression() const {
            return this->m_expression;
        }

    private:
        Token::Operator m_op;
        std::unique_ptr<ASTNode> m_expression;

        bool m_providerOperation = false;
    };

}