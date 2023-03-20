#pragma once

#include <pl/core/ast/ast_node.hpp>

namespace pl::core::ast {

    class ASTNodeLiteral : public ASTNode {
    public:
        explicit ASTNodeLiteral(Token::Literal literal) : ASTNode(), m_literal(std::move(literal)) { }

        ASTNodeLiteral(const ASTNodeLiteral &) = default;

        [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
            return std::unique_ptr<ASTNode>(new ASTNodeLiteral(*this));
        }

        [[nodiscard]] const auto &getValue() const {
            return this->m_literal;
        }

        void emit(instr::Bytecode &bytecode, instr::BytecodeEmitter &emitter) override {
            hlp::unused(bytecode);
            if(Token::isUnsigned(m_literal.getType())) {
                u16 symbol = bytecode.getSymbolTable().newUnsignedInteger(m_literal.toUnsigned());
                emitter.load_symbol(symbol);
            } else if(Token::isSigned(m_literal.getType())) {
                u16 symbol = bytecode.getSymbolTable().newSignedInteger((i64) m_literal.toSigned());
                emitter.load_symbol(symbol);
            } else {
                //core::err::P0002.throwError("Don't know how to emit literal", {}, 0);
            }
        }

    private:
        Token::Literal m_literal;
    };

}