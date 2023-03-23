#pragma once

#include <pl/core/ast/ast_node.hpp>
#include <pl/core/ast/ast_node_literal.hpp>
#include <pl/core/ast/ast_node_parameter_pack.hpp>

#include <pl/patterns/pattern_pointer.hpp>
#include <pl/patterns/pattern_unsigned.hpp>
#include <pl/patterns/pattern_signed.hpp>
#include <pl/patterns/pattern_float.hpp>
#include <pl/patterns/pattern_boolean.hpp>
#include <pl/patterns/pattern_character.hpp>
#include <pl/patterns/pattern_wide_character.hpp>
#include <pl/patterns/pattern_string.hpp>
#include <pl/patterns/pattern_wide_string.hpp>
#include <pl/patterns/pattern_array_dynamic.hpp>
#include <pl/patterns/pattern_array_static.hpp>
#include <pl/patterns/pattern_struct.hpp>
#include <pl/patterns/pattern_union.hpp>
#include <pl/patterns/pattern_enum.hpp>
#include <pl/patterns/pattern_bitfield.hpp>
#include <pl/patterns/pattern_padding.hpp>

namespace pl::core::ast {

    class ASTNodeRValue : public ASTNode {
    public:
        using PathSegment = std::variant<std::string, std::unique_ptr<ASTNode>>;
        using Path        = std::vector<PathSegment>;

        explicit ASTNodeRValue(Path &&path) : ASTNode(), m_path(std::move(path)) { }

        ASTNodeRValue(const ASTNodeRValue &other) : ASTNode(other) {
            for (auto &part : other.m_path) {
                if (auto stringPart = std::get_if<std::string>(&part); stringPart != nullptr)
                    this->m_path.emplace_back(*stringPart);
                else if (auto nodePart = std::get_if<std::unique_ptr<ASTNode>>(&part); nodePart != nullptr)
                    this->m_path.emplace_back((*nodePart)->clone());
            }
        }

        [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
            return std::unique_ptr<ASTNode>(new ASTNodeRValue(*this));
        }

        [[nodiscard]] const Path &getPath() const {
            return this->m_path;
        }

        void emit(instr::Bytecode &bytecode, instr::BytecodeEmitter &emitter) override {
            wolv::util::unused(bytecode);
            if(m_path.size() == 1) {
                auto name = std::get<std::string>(m_path[0]);
                emitter.flags.ctor ? emitter.load_field(name, true) : emitter.load_local(name);
            }
        }

    private:
        Path m_path;

        void readVariable(VirtualMachine *evaluator, auto &value, ptrn::Pattern *variablePattern) const {
            constexpr bool isString = std::same_as<std::remove_cvref_t<decltype(value)>, std::string>;

            if constexpr (isString) {
                value.resize(variablePattern->getSize());
                evaluator->readData(variablePattern->getOffset(), value.data(), value.size(), variablePattern->getSection());
            } else {
                evaluator->readData(variablePattern->getOffset(), &value, variablePattern->getSize(), variablePattern->getSection());
            }

            if constexpr (!isString)
                value = hlp::changeEndianess(value, variablePattern->getSize(), variablePattern->getEndian());
        }
    };

}