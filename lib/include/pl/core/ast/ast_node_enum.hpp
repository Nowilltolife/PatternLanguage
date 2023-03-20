#pragma once

#include <pl/core/ast/ast_node.hpp>
#include <pl/core/ast/ast_node_attribute.hpp>

#include <pl/patterns/pattern_enum.hpp>

namespace pl::core::ast {

    class ASTNodeEnum : public ASTNode,
                        public Attributable {
    public:
        explicit ASTNodeEnum(std::unique_ptr<ASTNode> &&underlyingType) : ASTNode(), m_underlyingType(std::move(underlyingType)) { }

        ASTNodeEnum(const ASTNodeEnum &other) : ASTNode(other), Attributable(other) {
            for (const auto &[name, expr] : other.getEntries()) {
                this->m_entries[name] = { expr.first->clone(), expr.second->clone() };
            }
            this->m_underlyingType = other.m_underlyingType->clone();
        }

        [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
            return std::unique_ptr<ASTNode>(new ASTNodeEnum(*this));
        }

        [[nodiscard]] const std::map<std::string, std::pair<std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>>> &getEntries() const { return this->m_entries; }
        void addEntry(const std::string &name, std::unique_ptr<ASTNode> &&minExpr, std::unique_ptr<ASTNode> &&maxExpr) {
            this->m_entries[name] = { std::move(minExpr), std::move(maxExpr) };
        }

        [[nodiscard]] const std::unique_ptr<ASTNode> &getUnderlyingType() { return this->m_underlyingType; }

    private:
        std::map<std::string, std::pair<std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>>> m_entries;
        std::unique_ptr<ASTNode> m_underlyingType;
    };

}