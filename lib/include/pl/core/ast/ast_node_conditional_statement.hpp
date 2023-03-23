#pragma once

#include <pl/core/ast/ast_node.hpp>
#include <pl/core/ast/ast_node_literal.hpp>

namespace pl::core::ast {

    class ASTNodeConditionalStatement : public ASTNode {
    public:
        explicit ASTNodeConditionalStatement(std::unique_ptr<ASTNode> condition, std::vector<std::unique_ptr<ASTNode>> &&trueBody, std::vector<std::unique_ptr<ASTNode>> &&falseBody)
            : ASTNode(), m_condition(std::move(condition)), m_trueBody(std::move(trueBody)), m_falseBody(std::move(falseBody)) { }


        ASTNodeConditionalStatement(const ASTNodeConditionalStatement &other) : ASTNode(other) {
            this->m_condition = other.m_condition->clone();

            for (auto &statement : other.m_trueBody)
                this->m_trueBody.push_back(statement->clone());
            for (auto &statement : other.m_falseBody)
                this->m_falseBody.push_back(statement->clone());
        }

        [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
            return std::unique_ptr<ASTNode>(new ASTNodeConditionalStatement(*this));
        }

        void emit(instr::Bytecode &bytecode, instr::BytecodeEmitter &emitter) override {
            m_condition->emit(bytecode, emitter);
            emitter.cmp(); // if true, skip next instruction
            auto elseLabel = emitter.label();
            emitter.jmp(elseLabel); // jump to else block
            for (const auto &item: m_trueBody)
                item->emit(bytecode, emitter);
            emitter.place_label(elseLabel); // else block
            for (const auto &item: m_falseBody)
                item->emit(bytecode, emitter);
            emitter.resolve_label(elseLabel);
        }

        [[nodiscard]] const std::unique_ptr<ASTNode> &getCondition() const { return m_condition; }
        [[nodiscard]] const std::vector<std::unique_ptr<ASTNode>> &getTrueBody() const { return m_trueBody; }
        [[nodiscard]] const std::vector<std::unique_ptr<ASTNode>> &getFalseBody() const { return m_falseBody; }

    private:
        std::unique_ptr<ASTNode> m_condition;
        std::vector<std::unique_ptr<ASTNode>> m_trueBody, m_falseBody;
    };

}