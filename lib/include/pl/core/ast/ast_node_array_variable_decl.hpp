#pragma once

#include <pl/core/ast/ast_node.hpp>
#include <pl/core/ast/ast_node_attribute.hpp>
#include <pl/core/ast/ast_node_literal.hpp>
#include <pl/core/ast/ast_node_builtin_type.hpp>
#include <pl/core/ast/ast_node_while_statement.hpp>

#include <pl/patterns/pattern_padding.hpp>
#include <pl/patterns/pattern_character.hpp>
#include <pl/patterns/pattern_wide_character.hpp>
#include <pl/patterns/pattern_string.hpp>
#include <pl/patterns/pattern_wide_string.hpp>
#include <pl/patterns/pattern_array_dynamic.hpp>
#include <pl/patterns/pattern_array_static.hpp>

namespace pl::core::ast {

    class ASTNodeArrayVariableDecl : public ASTNode,
                                     public Attributable {
    public:
        ASTNodeArrayVariableDecl(std::string name, std::shared_ptr<ASTNodeTypeDecl> type, std::unique_ptr<ASTNode> &&size, std::unique_ptr<ASTNode> &&placementOffset = {}, std::unique_ptr<ASTNode> &&placementSection = {}, bool constant = false)
            : ASTNode(), m_name(std::move(name)), m_type(std::move(type)), m_size(std::move(size)), m_placementOffset(std::move(placementOffset)), m_placementSection(std::move(placementSection)), m_constant(constant) { }

        ASTNodeArrayVariableDecl(const ASTNodeArrayVariableDecl &other) : ASTNode(other), Attributable(other) {
            this->m_name = other.m_name;
            if (other.m_type->isForwardDeclared())
                this->m_type = other.m_type;
            else
                this->m_type = std::shared_ptr<ASTNodeTypeDecl>(static_cast<ASTNodeTypeDecl*>(other.m_type->clone().release()));

            if (other.m_size != nullptr)
                this->m_size = other.m_size->clone();

            if (other.m_placementOffset != nullptr)
                this->m_placementOffset = other.m_placementOffset->clone();

            if (other.m_placementSection != nullptr)
                this->m_placementSection = other.m_placementSection->clone();
        }

        [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
            return std::unique_ptr<ASTNode>(new ASTNodeArrayVariableDecl(*this));
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

        [[nodiscard]] const std::unique_ptr<ASTNode> &getPlacementOffset() const {
            return this->m_placementOffset;
        }

        [[nodiscard]] bool isConstant() const {
            return this->m_constant;
        }

        void emit(instr::Bytecode &bytecode, instr::BytecodeEmitter &emitter) override {
            this->m_size->emit(bytecode, emitter);
            auto &type = getType()->getType();
            //auto &placementOffset = var->getPlacementOffset();
            const std::string& name = getName();
            auto [typeInfo, typeName] = emitter.getTypeInfo(type);
            if(!emitter.flags.ctor) {
                emitter.read_array(typeInfo);
                emitter.local(name, typeName);
                emitter.dup();
                emitter.store_local(name, typeName);
                emitter.export_(name);
            } else {
                emitter.read_array(typeInfo);
                emitter.store_field(name, typeName, true);
            }
        }

    private:
        std::string m_name;
        std::shared_ptr<ASTNodeTypeDecl> m_type;
        std::unique_ptr<ASTNode> m_size;
        std::unique_ptr<ASTNode> m_placementOffset, m_placementSection;
        bool m_constant;
    };

}