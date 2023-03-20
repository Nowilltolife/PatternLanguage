#pragma once

#include <pl/core/ast/ast_node.hpp>
#include <pl/core/ast/ast_node_attribute.hpp>

#include <pl/patterns/pattern_struct.hpp>

namespace pl::core::ast {

    class ASTNodeStruct : public ASTNode,
                          public Attributable {
    public:
        ASTNodeStruct() : ASTNode() { }

        ASTNodeStruct(const ASTNodeStruct &other) : ASTNode(other), Attributable(other) {
            for (const auto &otherMember : other.getMembers())
                this->m_members.push_back(otherMember->clone());
            for (const auto &otherInheritance : other.getInheritance())
                this->m_inheritance.push_back(otherInheritance->clone());
        }

        [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
            return std::unique_ptr<ASTNode>(new ASTNodeStruct(*this));
        }

        void emit(instr::Bytecode &bytecode, instr::BytecodeEmitter &emitter) override;

        [[nodiscard]] const std::vector<std::shared_ptr<ASTNode>> &getMembers() const { return this->m_members; }
        void addMember(std::shared_ptr<ASTNode> &&node) { this->m_members.push_back(std::move(node)); }

        [[nodiscard]] const std::vector<std::shared_ptr<ASTNode>> &getInheritance() const { return this->m_inheritance; }
        void addInheritance(std::shared_ptr<ASTNode> &&node) { this->m_inheritance.push_back(std::move(node)); }

    private:
        std::vector<std::shared_ptr<ASTNode>> m_members;
        std::vector<std::shared_ptr<ASTNode>> m_inheritance;
    };

}