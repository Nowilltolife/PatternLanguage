#pragma once

#include <pl/core/ast/ast_node.hpp>
#include <pl/core/ast/ast_node_literal.hpp>

#include <pl/patterns/pattern_pointer.hpp>
#include <pl/patterns/pattern_array_dynamic.hpp>


namespace pl::core::ast {

    class ASTNodeAttribute : public ASTNode {
    public:
        explicit ASTNodeAttribute(std::string attribute, std::vector<std::unique_ptr<ASTNode>> &&value = {})
            : ASTNode(), m_attribute(std::move(attribute)), m_value(std::move(value)) { }

        ~ASTNodeAttribute() override = default;

        ASTNodeAttribute(const ASTNodeAttribute &other) : ASTNode(other) {
            this->m_attribute = other.m_attribute;

            for (const auto &value : other.m_value)
                this->m_value.emplace_back(value->clone());
        }

        [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
            return std::unique_ptr<ASTNode>(new ASTNodeAttribute(*this));
        }

        [[nodiscard]] const std::string &getAttribute() const {
            return this->m_attribute;
        }

        [[nodiscard]] const std::vector<std::unique_ptr<ASTNode>> &getArguments() const {
            return this->m_value;
        }

    private:
        std::string m_attribute;
        std::vector<std::unique_ptr<ASTNode>> m_value;
    };


    class Attributable {
    protected:
        Attributable() = default;

        Attributable(const Attributable &other) {
            for (auto &attribute : other.m_attributes) {
                auto copy = attribute->clone();
                if (auto node = dynamic_cast<ASTNodeAttribute *>(copy.get())) {
                    this->m_attributes.push_back(std::unique_ptr<ASTNodeAttribute>(node));
                    (void)copy.release();
                }
            }
        }

    public:
        virtual void addAttribute(std::unique_ptr<ASTNodeAttribute> &&attribute) {
            this->m_attributes.push_back(std::move(attribute));
        }

        [[nodiscard]] const auto &getAttributes() const {
            return this->m_attributes;
        }

        [[nodiscard]] ASTNodeAttribute *getAttributeByName(const std::string &key) const {
            auto it = std::find_if(this->getAttributes().begin(), this->getAttributes().end(), [&](const std::unique_ptr<ASTNodeAttribute> &attribute) {
                return attribute->getAttribute() == key;
            });
            if (it == this->getAttributes().end())
                return nullptr;
            return it->get();
        }

        [[nodiscard]] bool hasAttribute(const std::string &key, bool needsParameter) const {
            return std::any_of(this->m_attributes.begin(), this->m_attributes.end(), [&](const std::unique_ptr<ASTNodeAttribute> &attribute) {
                if (attribute->getAttribute() == key) {
                    if (needsParameter && attribute->getArguments().empty())
                        err::E0008.throwError(fmt::format("Attribute '{}' expected a parameter.", key), fmt::format("Try [[{}(\"value\")]] instead.", key), attribute.get());
                    else if (!needsParameter && !attribute->getArguments().empty())
                        err::E0008.throwError(fmt::format("Attribute '{}' did not expect a parameter.", key), fmt::format("Try [[{}]] instead.", key), attribute.get());
                    else
                        return true;
                }

                return false;
            });
        }

        [[nodiscard]] const std::vector<std::unique_ptr<ASTNode>>& getAttributeArguments(const std::string &key) const {
            auto *attribute = this->getAttributeByName(key);
            if (attribute == nullptr) {
                static std::vector<std::unique_ptr<ASTNode>> empty;
                return empty;
            }
            return attribute->getArguments();
        }

        [[nodiscard]] std::shared_ptr<ASTNode> getFirstAttributeValue(const std::vector<std::string> &keys) const {
            for (const auto &key : keys) {
                if (const auto &arguments = this->getAttributeArguments(key); !arguments.empty())
                    return arguments.front()->clone();
            }

            return nullptr;
        }

    private:
        std::vector<std::unique_ptr<ASTNodeAttribute>> m_attributes;
    };
}
