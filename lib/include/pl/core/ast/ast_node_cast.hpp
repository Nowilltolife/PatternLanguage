#pragma once

#include <pl/core/ast/ast_node.hpp>
#include <pl/core/ast/ast_node_literal.hpp>
#include <pl/core/ast/ast_node_builtin_type.hpp>

namespace pl::core::ast {

    class ASTNodeCast : public ASTNode {
    public:
        ASTNodeCast(std::unique_ptr<ASTNode> &&value, std::unique_ptr<ASTNode> &&type) : m_value(std::move(value)), m_type(std::move(type)) { }

        ASTNodeCast(const ASTNodeCast &other) : ASTNode(other) {
            this->m_value = other.m_value->clone();
            this->m_type  = other.m_type->clone();
        }

    private:
        template<typename T>
        T changeEndianess(Evaluator *evaluator, T value, size_t size, std::endian endian) const {
            if (endian == evaluator->getDefaultEndian())
                return value;

            if constexpr (std::endian::native == std::endian::little)
                return hlp::changeEndianess(value, size, std::endian::big);
            else
                return hlp::changeEndianess(value, size, std::endian::little);
        }

    public:

        [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
            return std::unique_ptr<ASTNode>(new ASTNodeCast(*this));
        }

    private:
        std::unique_ptr<ASTNode> m_value;
        std::unique_ptr<ASTNode> m_type;
    };

}