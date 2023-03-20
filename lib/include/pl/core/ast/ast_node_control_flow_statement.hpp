#pragma once

#include <pl/core/ast/ast_node.hpp>

namespace pl::core::ast {

    class ASTNodeControlFlowStatement : public ASTNode {
    public:
        explicit ASTNodeControlFlowStatement(ControlFlowStatement type, std::unique_ptr<ASTNode> &&rvalue) : m_type(type), m_rvalue(std::move(rvalue)) {
        }

        ASTNodeControlFlowStatement(const ASTNodeControlFlowStatement &other) : ASTNode(other) {
            this->m_type = other.m_type;

            if (other.m_rvalue != nullptr)
                this->m_rvalue = other.m_rvalue->clone();
            else
                this->m_rvalue = nullptr;
        }

        [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
            return std::unique_ptr<ASTNode>(new ASTNodeControlFlowStatement(*this));
        }

    private:
        ControlFlowStatement m_type;
        std::unique_ptr<ASTNode> m_rvalue;
    };

}