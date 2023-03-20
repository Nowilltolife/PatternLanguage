#pragma once

#include <pl/core/ast/ast_node.hpp>

namespace pl::core::ast {

    class ASTNodeFunctionDefinition : public ASTNode {
    public:
        ASTNodeFunctionDefinition(std::string name, std::vector<std::pair<std::string, std::unique_ptr<ASTNode>>> &&params, std::vector<std::unique_ptr<ASTNode>> &&body, std::optional<std::string> parameterPack, std::vector<std::unique_ptr<ASTNode>> &&defaultParameters)
            : m_name(std::move(name)), m_params(std::move(params)), m_body(std::move(body)), m_parameterPack(std::move(parameterPack)), m_defaultParameters(std::move(defaultParameters)) {
        }

        ASTNodeFunctionDefinition(const ASTNodeFunctionDefinition &other) : ASTNode(other) {
            this->m_name = other.m_name;
            this->m_parameterPack = other.m_parameterPack;

            for (const auto &[name, type] : other.m_params) {
                this->m_params.emplace_back(name, type->clone());
            }

            for (auto &statement : other.m_body) {
                this->m_body.push_back(statement->clone());
            }

            for (auto &statement : other.m_defaultParameters) {
                this->m_body.push_back(statement->clone());
            }
        }

        [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
            return std::unique_ptr<ASTNode>(new ASTNodeFunctionDefinition(*this));
        }

        [[nodiscard]] const std::string &getName() const {
            return this->m_name;
        }

        [[nodiscard]] const auto &getParams() const {
            return this->m_params;
        }

        [[nodiscard]] const auto &getBody() const {
            return this->m_body;
        }

        [[nodiscard]] const auto &getParameterPack() const {
            return this->m_parameterPack;
        }

        [[nodiscard]] const auto &getDefaultParameters() const {
            return this->m_defaultParameters;
        }


    private:
        std::string m_name;
        std::vector<std::pair<std::string, std::unique_ptr<ASTNode>>> m_params;
        std::vector<std::unique_ptr<ASTNode>> m_body;
        std::optional<std::string> m_parameterPack;
        std::vector<std::unique_ptr<ASTNode>> m_defaultParameters;
    };

}