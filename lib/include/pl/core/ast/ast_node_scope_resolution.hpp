#pragma once

#include <pl/core/ast/ast_node.hpp>

namespace pl::core::ast {

    class ASTNodeScopeResolution : public ASTNode {
    public:
        explicit ASTNodeScopeResolution(std::shared_ptr<ASTNode> &&type, std::string name) : ASTNode(), m_type(std::move(type)), m_name(std::move(name)) { }

        ASTNodeScopeResolution(const ASTNodeScopeResolution &other) : ASTNode(other) {
            this->m_type = other.m_type;
            this->m_name = other.m_name;
        }

        [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
            return std::unique_ptr<ASTNode>(new ASTNodeScopeResolution(*this));
        }

    private:
        std::shared_ptr<ASTNode> m_type;
        std::string m_name;
    };

}