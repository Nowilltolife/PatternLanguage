#pragma once

#include <pl/core/ast/ast_node.hpp>

namespace pl::core::ast {

    class ASTNodeCompoundStatement : public ASTNode {
    public:
        explicit ASTNodeCompoundStatement(std::vector<std::unique_ptr<ASTNode>> &&statements, bool newScope = false) : m_statements(std::move(statements)), m_newScope(newScope) {
        }

        ASTNodeCompoundStatement(const ASTNodeCompoundStatement &other) : ASTNode(other) {
            for (const auto &statement : other.m_statements) {
                this->m_statements.push_back(statement->clone());
            }

            this->m_newScope = other.m_newScope;
        }

        [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
            return std::unique_ptr<ASTNode>(new ASTNodeCompoundStatement(*this));
        }

        [[nodiscard]] const std::vector<std::unique_ptr<ASTNode>>& getStatements() const { return this->m_statements; }

        void emit(instr::Bytecode &bytecode, instr::BytecodeEmitter &emitter) override {
            for(auto &statement : this->m_statements) {
                statement->emit(bytecode, emitter);
            }
        }

    public:
        std::vector<std::unique_ptr<ASTNode>> m_statements;
        bool m_newScope = false;
    };

}