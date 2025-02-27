#pragma once

#include <pl/core/ast/ast_node.hpp>

#include <pl/core/ast/ast_node_variable_decl.hpp>

namespace pl::core::ast {

    class ASTNodeMultiVariableDecl : public ASTNode {
    public:
        explicit ASTNodeMultiVariableDecl(std::vector<std::shared_ptr<ASTNode>> &&variables) : m_variables(std::move(variables)) { }

        ASTNodeMultiVariableDecl(const ASTNodeMultiVariableDecl &other) : ASTNode(other) {
            for (auto &variable : other.m_variables)
                this->m_variables.push_back(variable->clone());
        }

        [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
            return std::unique_ptr<ASTNode>(new ASTNodeMultiVariableDecl(*this));
        }

        [[nodiscard]] const std::vector<std::shared_ptr<ASTNode>> &getVariables() {
            return this->m_variables;
        }

        [[nodiscard]] std::vector<std::shared_ptr<ptrn::Pattern>> createPatterns(Evaluator *evaluator) const override {
            evaluator->updateRuntime(this);

            std::vector<std::shared_ptr<ptrn::Pattern>> patterns;

            for (auto &node : this->m_variables) {
                auto newPatterns = node->createPatterns(evaluator);
                std::move(newPatterns.begin(), newPatterns.end(), std::back_inserter(patterns));
            }

            return patterns;
        }

        FunctionResult execute(Evaluator *evaluator) const override {
            evaluator->updateRuntime(this);

            for (auto &variable : this->m_variables) {
                auto variableDecl = dynamic_cast<ASTNodeVariableDecl *>(variable.get());
                auto variableType = variableDecl->getType()->evaluate(evaluator);

                evaluator->createVariable(variableDecl->getName(), variableType.get());
            }

            return std::nullopt;
        }

    private:
        std::vector<std::shared_ptr<ASTNode>> m_variables;
    };

}