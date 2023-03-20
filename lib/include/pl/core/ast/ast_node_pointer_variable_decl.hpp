#pragma once

#include <pl/core/ast/ast_node.hpp>
#include <pl/core/ast/ast_node_attribute.hpp>

#include <pl/patterns/pattern_pointer.hpp>

namespace pl::core::ast {

    class ASTNodePointerVariableDecl : public ASTNode,
                                       public Attributable {
    public:
        ASTNodePointerVariableDecl(std::string name, std::shared_ptr<ASTNode> type, std::shared_ptr<ASTNodeTypeDecl> sizeType, std::unique_ptr<ASTNode> &&placementOffset = nullptr, std::unique_ptr<ASTNode> &&placementSection = nullptr)
            : ASTNode(), m_name(std::move(name)), m_type(std::move(type)), m_sizeType(std::move(sizeType)), m_placementOffset(std::move(placementOffset)), m_placementSection(std::move(placementSection)) { }

        ASTNodePointerVariableDecl(const ASTNodePointerVariableDecl &other) : ASTNode(other), Attributable(other) {
            this->m_name     = other.m_name;
            this->m_type = std::shared_ptr<ASTNodeTypeDecl>(static_cast<ASTNodeTypeDecl*>(other.m_type->clone().release()));
            this->m_sizeType = other.m_sizeType;

            if (other.m_placementOffset != nullptr)
                this->m_placementOffset = other.m_placementOffset->clone();
            if (other.m_placementSection != nullptr)
                this->m_placementSection = other.m_placementSection->clone();
        }

        [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
            return std::unique_ptr<ASTNode>(new ASTNodePointerVariableDecl(*this));
        }

        [[nodiscard]] const std::string &getName() const { return this->m_name; }
        [[nodiscard]] constexpr const std::shared_ptr<ASTNode> &getType() const { return this->m_type; }
        [[nodiscard]] constexpr const std::shared_ptr<ASTNodeTypeDecl> &getSizeType() const { return this->m_sizeType; }
        [[nodiscard]] constexpr const std::unique_ptr<ASTNode> &getPlacementOffset() const { return this->m_placementOffset; }

    private:
        std::string m_name;
        std::shared_ptr<ASTNode> m_type;
        std::shared_ptr<ASTNodeTypeDecl> m_sizeType;
        std::unique_ptr<ASTNode> m_placementOffset, m_placementSection;
    };

}
