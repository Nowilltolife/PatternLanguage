#pragma once

#include <pl/patterns/pattern.hpp>

namespace pl::ptrn {

    class PatternUnsigned : public Pattern {
    public:
        PatternUnsigned(core::Evaluator *evaluator, u64 offset, size_t size)
            : Pattern(evaluator, offset, size) { }

        [[nodiscard]] std::unique_ptr<Pattern> clone() const override {
            return std::unique_ptr<Pattern>(new PatternUnsigned(*this));
        }

        [[nodiscard]] core::Token::Literal getValue() const override {
            return transformValue(data);
        }

        void setData(u128 d) {
            this->data = d;
        }

        [[nodiscard]] std::string getFormattedName() const override {
            return this->getTypeName();
        }

        [[nodiscard]] bool operator==(const Pattern &other) const override { return compareCommonProperties<decltype(*this)>(other); }

        void accept(PatternVisitor &v) override {
            v.visit(*this);
        }

        std::string formatDisplayValue() override {
            return Pattern::formatDisplayValue(fmt::format("{:d} (0x{:0{}X})", data, data, this->getSize() * 2), this->getValue());
        }

        [[nodiscard]] std::string toString() const override {
            auto value = this->getValue();
            auto result = fmt::format("{:d}", value.toUnsigned());

            return Pattern::formatDisplayValue(result, value);
        }
    private:
        u128 data = 0;
    };

}