#pragma once

#include <pl/core/ast/ast_node.hpp>
#include <pl/core/ast/ast_node_literal.hpp>
#include <pl/core/ast/ast_node_mathematical_expression.hpp>
#include <utility>

namespace pl::core::ast {

    struct MatchCase {
        std::unique_ptr<ASTNode> condition;
        std::vector<std::unique_ptr<ASTNode>> body;

        MatchCase() = default;

        MatchCase(std::unique_ptr<ASTNode> condition, std::vector<std::unique_ptr<ASTNode>> body)
            : condition(std::move(condition)), body(std::move(body)) { }

        MatchCase(const MatchCase &other) {
            this->condition = other.condition->clone();
            for (auto &statement : other.body)
                this->body.push_back(statement->clone());
        }

        MatchCase(MatchCase &&other) noexcept {
            this->condition = std::move(other.condition);
            this->body = std::move(other.body);
        }

        MatchCase &operator=(const MatchCase &other) {
            this->condition = other.condition->clone();
            for (auto &statement : other.body)
                this->body.push_back(statement->clone());
            return *this;
        }
    };

    class ASTNodeMatchStatement : public ASTNode {

    public:
        explicit ASTNodeMatchStatement(std::vector<MatchCase> cases, std::optional<MatchCase> defaultCase)
            : ASTNode(), m_cases(std::move(cases)), m_defaultCase(std::move(defaultCase)) { }

        ASTNodeMatchStatement(const ASTNodeMatchStatement &other) : ASTNode(other) {
            for (auto &matchCase : other.m_cases)
                this->m_cases.push_back(matchCase);
            if(other.m_defaultCase) {
                this->m_defaultCase.emplace(*other.m_defaultCase);
            } else {
                this->m_defaultCase = std::nullopt;
            }
        }

        [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
            return std::unique_ptr<ASTNode>(new ASTNodeMatchStatement(*this));
        }

        void emit(instr::Bytecode &bytecode, instr::BytecodeEmitter &emitter) override {
            auto endLabel = emitter.label();
            for (const auto &item: this->m_cases){
                item.condition->emit(bytecode, emitter);
                emitter.cmp();
                auto elseLabel = emitter.label();
                emitter.jmp(elseLabel);
                for (const auto &statement: item.body)
                    statement->emit(bytecode, emitter);
                emitter.jmp(endLabel);
                emitter.place_label(elseLabel);
                emitter.resolve_label(elseLabel);
            }
            if(this->m_defaultCase) {
                for (const auto &statement : this->m_defaultCase->body)
                    statement->emit(bytecode, emitter);
            }
            emitter.place_label(endLabel);
        }

    private:

        std::vector<MatchCase> m_cases;
        std::optional<MatchCase> m_defaultCase;
    };
}