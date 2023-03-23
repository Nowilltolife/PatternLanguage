#pragma once

#include <pl/core/token.hpp>
#include <pl/patterns/pattern.hpp>
#include <pl/core/errors/evaluator_errors.hpp>
#include <pl/core/bytecode/bytecode.hpp>

#include <pl/helpers/utils.hpp>
#include <pl/helpers/concepts.hpp>

#include <wolv/utils/guards.hpp>

namespace pl::ptrn { class Pattern; }

namespace pl::core::ast {

    class ASTNode : public Cloneable<ASTNode> {
    public:
        constexpr ASTNode() = default;

        constexpr virtual ~ASTNode() = default;

        constexpr ASTNode(const ASTNode &) = default;

        [[nodiscard]] constexpr u32 getLine() const { return this->m_line; }
        [[nodiscard]] constexpr u32 getColumn() const { return this->m_column; }

        [[maybe_unused]] constexpr void setSourceLocation(u32 line, u32 column) {
            this->m_line = line;
            this->m_column = column;
        }

        virtual void emit(instr::Bytecode& bytecode, instr::BytecodeEmitter& emitter) {
            wolv::util::unused(bytecode, emitter);
            err::E0001.throwError("Non implemented instr emitter.", "This is a evaluator bug!", this);
        }

        void setDocComment(const std::string &comment) {
            this->m_docComment = comment;
        }

        [[nodiscard]] const std::string &getDocComment() const {
            return this->m_docComment;
        }

        void setShouldDocument(bool shouldDocument) {
            this->m_document = shouldDocument;
        }

        [[nodiscard]] bool shouldDocument() const {
            return this->m_document;
        }

    private:
        u32 m_line = 1;
        u32 m_column = 1;

        std::string m_docComment;
        bool m_document = false;
    };

}