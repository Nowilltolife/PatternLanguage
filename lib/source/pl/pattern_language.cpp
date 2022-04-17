#include "pl/pattern_language.hpp"

#include "pl/preprocessor.hpp"
#include "pl/lexer.hpp"
#include "pl/parser.hpp"
#include "pl/validator.hpp"
#include "pl/evaluator.hpp"

#include "helpers/fs.hpp"
#include "helpers/file.hpp"

namespace pl {

    class Pattern;

    PatternLanguage::PatternLanguage() {
        this->m_internals.preprocessor  = new Preprocessor();
        this->m_internals.lexer         = new Lexer();
        this->m_internals.parser        = new Parser();
        this->m_internals.validator     = new Validator();
        this->m_internals.evaluator     = new Evaluator();
    }

    PatternLanguage::~PatternLanguage() {
        delete this->m_internals.preprocessor;
        delete this->m_internals.lexer;
        delete this->m_internals.parser;
        delete this->m_internals.validator;
        delete this->m_internals.evaluator;
    }

    std::optional<std::vector<std::shared_ptr<ASTNode>>> PatternLanguage::parseString(const std::string &code) {
        auto preprocessedCode = this->m_internals.preprocessor->preprocess(*this, code);
        if (!preprocessedCode.has_value()) {
            this->m_currError = this->m_internals.preprocessor->getError();
            return std::nullopt;
        }

        auto tokens = this->m_internals.lexer->lex(preprocessedCode.value());
        if (!tokens.has_value()) {
            this->m_currError = this->m_internals.lexer->getError();
            return std::nullopt;
        }

        auto ast = this->m_internals.parser->parse(tokens.value());
        if (!ast.has_value()) {
            this->m_currError = this->m_internals.parser->getError();
            return std::nullopt;
        }

        if (!this->m_internals.validator->validate(*ast)) {
            this->m_currError = this->m_internals.validator->getError();

            return std::nullopt;
        }

        return ast;
    }

    bool PatternLanguage::executeString(const std::string &code, const std::map<std::string, Token::Literal> &envVars, const std::map<std::string, Token::Literal> &inVariables, bool checkResult) {
        this->m_running = true;
        PL_ON_SCOPE_EXIT { this->m_running = false; };

        PL_ON_SCOPE_EXIT {
            if (this->m_currError.has_value()) {
                const auto &error = this->m_currError.value();

                if (error.getLineNumber() > 0)
                    this->m_internals.evaluator->getConsole().log(LogConsole::Level::Error, fmt::format("{}: {}", error.getLineNumber(), error.what()));
                else
                    this->m_internals.evaluator->getConsole().log(LogConsole::Level::Error, error.what());
            }
        };

        this->m_currError.reset();
        this->m_internals.evaluator->getConsole().clear();
        this->m_internals.evaluator->setDefaultEndian(std::endian::native);
        this->m_internals.evaluator->setEvaluationDepth(32);
        this->m_internals.evaluator->setArrayLimit(0x1000);
        this->m_internals.evaluator->setPatternLimit(0x2000);
        this->m_internals.evaluator->setLoopLimit(0x1000);
        this->m_internals.evaluator->setInVariables(inVariables);

        for (const auto &[name, value] : envVars)
            this->m_internals.evaluator->setEnvVariable(name, value);

        this->m_currAST.clear();

        {
            auto ast = this->parseString(code);
            if (!ast)
                return false;

            this->m_currAST = std::move(ast.value());
        }


        auto patterns = this->m_internals.evaluator->evaluate(this->m_currAST);
        if (!patterns.has_value()) {
            this->m_currError = this->m_internals.evaluator->getConsole().getLastHardError();
            return false;
        }

        if (auto mainResult = this->m_internals.evaluator->getMainResult(); checkResult && mainResult.has_value()) {
            auto returnCode = Token::literalToSigned(*mainResult);

            if (returnCode != 0) {
                this->m_currError = PatternLanguageError(0, fmt::format("non-success value returned from main: {}", returnCode));

                return false;
            }
        }

        this->m_patterns = std::move(patterns.value());

        return true;
    }

    bool PatternLanguage::executeFile(const std::fs::path &path, const std::map<std::string, Token::Literal> &envVars, const std::map<std::string, Token::Literal> &inVariables) {
        fs::File file(path, fs::File::Mode::Read);

        return this->executeString(file.readString(), envVars, inVariables, true);
    }

    std::pair<bool, std::optional<Token::Literal>> PatternLanguage::executeFunction(const std::string &code) {

        auto functionContent = fmt::format("fn main() {{ {0} }};", code);

        auto success = this->executeString(functionContent, {}, {}, false);
        auto result  = this->m_internals.evaluator->getMainResult();

        return { success, std::move(result) };
    }

    void PatternLanguage::abort() {
        this->m_internals.evaluator->abort();
    }

    void PatternLanguage::setIncludePaths(std::vector<std::fs::path> paths) {
        this->m_internals.preprocessor->setIncludePaths(std::move(paths));
    }

    void PatternLanguage::addPragma(const std::string &name, const api::PragmaHandler &callback) {
        this->m_internals.preprocessor->addPragmaHandler(name, callback);
    }

    void PatternLanguage::removePragma(const std::string &name) {
        this->m_internals.preprocessor->removePragmaHandler(name);
    }

    void PatternLanguage::setDataSource(std::function<void(u64, u8*, size_t)> readFunction, u64 baseAddress, u64 size) {
        this->m_internals.evaluator->setDataSource(std::move(readFunction), baseAddress, size);
    }

    void PatternLanguage::setDataBaseAddress(u64 baseAddress) {
        this->m_internals.evaluator->setDataBaseAddress(baseAddress);
    }

    void PatternLanguage::setDataSize(u64 size) {
        this->m_internals.evaluator->setDataSize(size);
    }

    void PatternLanguage::setDangerousFunctionCallHandler(std::function<bool()> callback) {
        this->m_internals.evaluator->setDangerousFunctionCallHandler(std::move(callback));
    }

    const std::vector<std::shared_ptr<ASTNode>> &PatternLanguage::getCurrentAST() const {
        return this->m_currAST;
    }

    [[nodiscard]] std::map<std::string, Token::Literal> PatternLanguage::getOutVariables() const {
        return this->m_internals.evaluator->getOutVariables();
    }


    const std::vector<std::pair<LogConsole::Level, std::string>> &PatternLanguage::getConsoleLog() {
        return this->m_internals.evaluator->getConsole().getLog();
    }

    const std::optional<PatternLanguageError> &PatternLanguage::getError() {
        return this->m_currError;
    }

    u32 PatternLanguage::getCreatedPatternCount() {
        return this->m_internals.evaluator->getPatternCount();
    }

    u32 PatternLanguage::getMaximumPatternCount() {
        return this->m_internals.evaluator->getPatternLimit();
    }


    void PatternLanguage::reset() {
        this->m_patterns.clear();

        this->m_currAST.clear();
    }


    static std::string getFunctionName(const api::Namespace &ns, const std::string &name) {
        std::string functionName;
        for (auto &scope : ns)
            functionName += scope + "::";

        functionName += name;

        return functionName;
    }

    void PatternLanguage::addFunction(const api::Namespace &ns, const std::string &name, api::FunctionParameterCount parameterCount, const api::FunctionCallback &func) {
        this->m_internals.evaluator->addBuiltinFunction(getFunctionName(ns, name), parameterCount, { }, func, false);
    }

    void PatternLanguage::addDangerousFunction(const api::Namespace &ns, const std::string &name, api::FunctionParameterCount parameterCount, const api::FunctionCallback &func) {
        this->m_internals.evaluator->addBuiltinFunction(getFunctionName(ns, name), parameterCount, { }, func, true);
    }

}