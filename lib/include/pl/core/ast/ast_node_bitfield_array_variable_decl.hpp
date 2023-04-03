#pragma once

#include <pl/core/ast/ast_node.hpp>
#include <pl/core/ast/ast_node_attribute.hpp>
#include <pl/core/ast/ast_node_bitfield.hpp>
#include <pl/core/ast/ast_node_bitfield_field.hpp>
#include <pl/core/ast/ast_node_literal.hpp>
#include <pl/core/ast/ast_node_type_decl.hpp>
#include <pl/core/ast/ast_node_while_statement.hpp>

#include <pl/patterns/pattern_array_dynamic.hpp>

namespace pl::core::ast {

    class ASTNodeBitfieldArrayVariableDecl : public ASTNode,
                                             public Attributable {
    public:
        ASTNodeBitfieldArrayVariableDecl(std::string name, std::shared_ptr<ASTNodeTypeDecl> type, std::unique_ptr<ASTNode> &&size)
            : ASTNode(), m_name(std::move(name)), m_type(std::move(type)), m_size(std::move(size)) { }

        ASTNodeBitfieldArrayVariableDecl(const ASTNodeBitfieldArrayVariableDecl &other) : ASTNode(other), Attributable(other) {
            this->m_name = other.m_name;
            if (other.m_type->isForwardDeclared())
                this->m_type = other.m_type;
            else
                this->m_type = std::shared_ptr<ASTNodeTypeDecl>(static_cast<ASTNodeTypeDecl*>(other.m_type->clone().release()));

            if (other.m_size != nullptr)
                this->m_size = other.m_size->clone();
        }

        [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
            return std::unique_ptr<ASTNode>(new ASTNodeBitfieldArrayVariableDecl(*this));
        }

        [[nodiscard]] const std::string &getName() const {
            return this->m_name;
        }

        [[nodiscard]] const std::shared_ptr<ASTNodeTypeDecl> &getType() const {
            return this->m_type;
        }

        [[nodiscard]] const std::unique_ptr<ASTNode> &getSize() const {
            return this->m_size;
        }

    private:
        std::string m_name;
        std::shared_ptr<ASTNodeTypeDecl> m_type;
        std::unique_ptr<ASTNode> m_size;

    };

}