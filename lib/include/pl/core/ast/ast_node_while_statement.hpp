#pragma once

#include <pl/core/ast/ast_node.hpp>
#include <pl/core/ast/ast_node_literal.hpp>

namespace pl::core::ast {

    class ASTNodeWhileStatement : public ASTNode {
    public:
        explicit ASTNodeWhileStatement(std::unique_ptr<ASTNode> &&condition, std::vector<std::unique_ptr<ASTNode>> &&body, std::unique_ptr<ASTNode> &&postExpression = nullptr)
            : ASTNode(), m_condition(std::move(condition)), m_body(std::move(body)), m_postExpression(std::move(postExpression)) { }

        ASTNodeWhileStatement(const ASTNodeWhileStatement &other) : ASTNode(other) {
            this->m_condition = other.m_condition->clone();

            for (auto &statement : other.m_body)
                this->m_body.push_back(statement->clone());

            if (other.m_postExpression != nullptr)
                this->m_postExpression = other.m_postExpression->clone();
            else
                this->m_postExpression = nullptr;
        }

        [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
            return std::unique_ptr<ASTNode>(new ASTNodeWhileStatement(*this));
        }

        [[nodiscard]] const std::unique_ptr<ASTNode> &getCondition() const {
            return this->m_condition;
        }

        [[nodiscard]] const std::vector<std::unique_ptr<ASTNode>> &getBody() const {
            return this->m_body;
        }

    private:
        std::unique_ptr<ASTNode> m_condition;
        std::vector<std::unique_ptr<ASTNode>> m_body;
        std::unique_ptr<ASTNode> m_postExpression;
    };

}