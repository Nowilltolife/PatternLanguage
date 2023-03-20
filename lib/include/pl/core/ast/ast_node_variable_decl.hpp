#pragma once

#include <pl/core/ast/ast_node.hpp>
#include <pl/core/ast/ast_node_attribute.hpp>
#include <pl/core/ast/ast_node_literal.hpp>
#include <pl/core/ast/ast_node_type_decl.hpp>

namespace pl::core::ast {

    class ASTNodeVariableDecl : public ASTNode,
                                public Attributable {
    public:
        ASTNodeVariableDecl(std::string name, std::shared_ptr<ASTNodeTypeDecl> type, std::unique_ptr<ASTNode> &&placementOffset = nullptr, std::unique_ptr<ASTNode> &&placementSection = nullptr, bool inVariable = false, bool outVariable = false, bool constant = false)
            : ASTNode(), m_name(std::move(name)), m_type(std::move(type)), m_placementOffset(std::move(placementOffset)), m_placementSection(std::move(placementSection)), m_inVariable(inVariable), m_outVariable(outVariable), m_constant(constant) { }

        ASTNodeVariableDecl(const ASTNodeVariableDecl &other) : ASTNode(other), Attributable(other) {
            this->m_name = other.m_name;
            this->m_type = std::shared_ptr<ASTNodeTypeDecl>(static_cast<ASTNodeTypeDecl*>(other.m_type->clone().release()));

            if (other.m_placementOffset != nullptr)
                this->m_placementOffset = other.m_placementOffset->clone();

            if (other.m_placementSection != nullptr)
                this->m_placementSection = other.m_placementSection->clone();

            this->m_inVariable  = other.m_inVariable;
            this->m_outVariable = other.m_outVariable;
            this->m_constant    = other.m_constant;
        }

        [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
            return std::unique_ptr<ASTNode>(new ASTNodeVariableDecl(*this));
        }

        [[nodiscard]] const std::string &getName() const { return this->m_name; }
        [[nodiscard]] constexpr const std::shared_ptr<ASTNodeTypeDecl> &getType() const { return this->m_type; }
        [[nodiscard]] constexpr const std::unique_ptr<ASTNode> &getPlacementOffset() const { return this->m_placementOffset; }

        [[nodiscard]] constexpr bool isInVariable() const { return this->m_inVariable; }
        [[nodiscard]] constexpr bool isOutVariable() const { return this->m_outVariable; }

        void emit(instr::Bytecode &bytecode, instr::BytecodeEmitter &emitter) override;

        [[nodiscard]] bool isConstant() const {
            return this->m_constant;
        }

    private:
        std::string m_name;
        std::shared_ptr<ASTNodeTypeDecl> m_type;
        std::unique_ptr<ASTNode> m_placementOffset, m_placementSection;

        bool m_inVariable = false, m_outVariable = false;
        bool m_constant = false;
    };

}