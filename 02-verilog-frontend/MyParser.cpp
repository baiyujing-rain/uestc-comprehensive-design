#include "MyParser.h"

#include <cctype>
#include <sstream>
#include <stdexcept>

namespace {

bool IsIdentifierStart(char ch)
{
    return std::isalpha(static_cast<unsigned char>(ch)) != 0 || ch == '_' || ch == '$';
}

bool IsIdentifierChar(char ch)
{
    return std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '_' || ch == '$';
}

bool IsNumberDigit(char ch)
{
    return std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '_' || ch == '\'' ||
           ch == 'x' || ch == 'X' || ch == 'z' || ch == 'Z';
}

} // namespace

IdentifierExprNode::IdentifierExprNode(const std::string &name)
    : ExprNode(Kind::Identifier), name(name) {}

NumberExprNode::NumberExprNode(const std::string &value)
    : ExprNode(Kind::Number), value(value) {}

UnaryExprNode::UnaryExprNode(char op, std::unique_ptr<ExprNode> operand)
    : ExprNode(Kind::Unary), op(op), operand(std::move(operand)) {}

BinaryExprNode::BinaryExprNode(std::unique_ptr<ExprNode> left, char op, std::unique_ptr<ExprNode> right)
    : ExprNode(Kind::Binary), left(std::move(left)), op(op), right(std::move(right)) {}

MuxExprNode::MuxExprNode(std::unique_ptr<ExprNode> condition,
                         std::unique_ptr<ExprNode> true_expr,
                         std::unique_ptr<ExprNode> false_expr)
    : ExprNode(Kind::Mux), condition(std::move(condition)),
      trueExpr(std::move(true_expr)), falseExpr(std::move(false_expr)) {}

PortNode::PortNode(const std::string &name, PWire::PortType direction)
    : name(name), direction(direction) {}

WireNode::WireNode(const std::string &name) : name(name) {}

AssignNode::AssignNode(const std::string &lhs, std::unique_ptr<ExprNode> rhs_expr)
    : lhs(lhs), rhsExpr(std::move(rhs_expr)) {}

ModuleNode::ModuleNode(const std::string &name) : name(name) {}

void ModuleNode::add_port(std::unique_ptr<PortNode> port)
{
    ports.push_back(std::move(port));
}

void ModuleNode::add_statement(std::unique_ptr<AstNode> statement)
{
    statements.push_back(std::move(statement));
}

void ModuleNode::update_port_direction(const std::string &name, PWire::PortType direction)
{
    for (size_t i = 0; i < ports.size(); ++i) {
        if (ports[i]->name == name) {
            ports[i]->direction = direction;
            return;
        }
    }
    add_port(std::unique_ptr<PortNode>(new PortNode(name, direction)));
}

Token::Token() : type(TokenType::EndOfInput), value(""), line(0), column(0) {}

Token::Token(TokenType type, const std::string &value, size_t line, size_t column)
    : type(type), value(value), line(line), column(column) {}

Lexer::Lexer(const std::string &input)
    : input(input), position(0), line(1), column(1)
{
    keywords.insert("module");
    keywords.insert("endmodule");
    keywords.insert("input");
    keywords.insert("output");
    keywords.insert("inout");
    keywords.insert("wire");
    keywords.insert("assign");
}

char Lexer::peek(size_t offset) const
{
    size_t index = position + offset;
    return index >= input.size() ? '\0' : input[index];
}

char Lexer::get()
{
    if (position >= input.size()) {
        return '\0';
    }
    char ch = input[position++];
    if (ch == '\n') {
        ++line;
        column = 1;
    } else {
        ++column;
    }
    return ch;
}

bool Lexer::is_keyword(const std::string &value) const
{
    return keywords.find(value) != keywords.end();
}

void Lexer::skip_whitespace_and_comments()
{
    for (;;) {
        while (std::isspace(static_cast<unsigned char>(peek())) != 0) {
            get();
        }
        if (peek() == '/' && peek(1) == '/') {
            while (peek() != '\0' && peek() != '\n') {
                get();
            }
            continue;
        }
        if (peek() == '/' && peek(1) == '*') {
            get();
            get();
            while (peek() != '\0' && !(peek() == '*' && peek(1) == '/')) {
                get();
            }
            if (peek() == '*' && peek(1) == '/') {
                get();
                get();
            }
            continue;
        }
        break;
    }
}

Token Lexer::read_identifier_or_keyword()
{
    size_t start_line = line;
    size_t start_column = column;
    std::string value;
    while (IsIdentifierChar(peek())) {
        value.push_back(get());
    }
    return Token(is_keyword(value) ? TokenType::Keyword : TokenType::Identifier,
                 value, start_line, start_column);
}

Token Lexer::read_number()
{
    size_t start_line = line;
    size_t start_column = column;
    std::string value;
    if (peek() == '\'') {
        value.push_back(get());
    } else {
        while (std::isdigit(static_cast<unsigned char>(peek())) != 0 || peek() == '_') {
            value.push_back(get());
        }
    }
    if (peek() == '\'') {
        value.push_back(get());
        if (peek() == 's' || peek() == 'S') {
            value.push_back(get());
        }
        if (std::isalpha(static_cast<unsigned char>(peek())) != 0) {
            value.push_back(get());
        }
        while (IsNumberDigit(peek())) {
            value.push_back(get());
        }
    }
    return Token(TokenType::Number, value, start_line, start_column);
}

Token Lexer::NextToken()
{
    skip_whitespace_and_comments();
    if (position >= input.size()) {
        return Token(TokenType::EndOfInput, "", line, column);
    }
    if (IsIdentifierStart(peek())) {
        return read_identifier_or_keyword();
    }
    if (std::isdigit(static_cast<unsigned char>(peek())) != 0 || peek() == '\'') {
        return read_number();
    }

    size_t start_line = line;
    size_t start_column = column;
    char ch = get();
    switch (ch) {
    case '(':
    case ')':
    case ',':
    case ';':
    case '=':
    case '~':
    case '&':
    case '|':
    case '^':
    case '?':
    case ':':
        return Token(TokenType::Symbol, std::string(1, ch), start_line, start_column);
    default:
        break;
    }
    std::ostringstream oss;
    oss << "Unsupported character '" << ch << "' at line " << start_line << ", column " << start_column;
    throw std::runtime_error(oss.str());
}

std::vector<Token> Lexer::Tokenize()
{
    std::vector<Token> result;
    for (;;) {
        Token token = NextToken();
        result.push_back(token);
        if (token.type == TokenType::EndOfInput) {
            break;
        }
    }
    return result;
}

Parser::Parser(Lexer &lexer) : lexer(lexer), currentTokenIndex(0) {}

const Token &Parser::current_token() const
{
    return currentTokenIndex >= tokens.size() ? tokens.back() : tokens[currentTokenIndex];
}

const Token &Parser::peek_token(size_t offset) const
{
    size_t index = currentTokenIndex + offset;
    return index >= tokens.size() ? tokens.back() : tokens[index];
}

bool Parser::is_end() const
{
    return current_token().type == TokenType::EndOfInput;
}

void Parser::consume()
{
    if (!is_end()) {
        ++currentTokenIndex;
    }
}

bool Parser::match_symbol(const std::string &value)
{
    if (current_token().type == TokenType::Symbol && current_token().value == value) {
        consume();
        return true;
    }
    return false;
}

bool Parser::match_keyword(const std::string &value)
{
    if (current_token().type == TokenType::Keyword && current_token().value == value) {
        consume();
        return true;
    }
    return false;
}

void Parser::expect_symbol(const std::string &value)
{
    if (!match_symbol(value)) {
        throw_parse_error("Expected '" + value + "'");
    }
}

std::string Parser::expect_identifier(const std::string &message)
{
    if (current_token().type != TokenType::Identifier) {
        throw_parse_error(message);
    }
    std::string value = current_token().value;
    consume();
    return value;
}

PWire::PortType Parser::parse_port_direction()
{
    if (match_keyword("input")) return PWire::PortType::Input;
    if (match_keyword("output")) return PWire::PortType::Output;
    if (match_keyword("inout")) return PWire::PortType::Inout;
    throw_parse_error("Expected port direction");
}

[[noreturn]] void Parser::throw_parse_error(const std::string &message) const
{
    const Token &token = current_token();
    std::ostringstream oss;
    oss << message << " at line " << token.line << ", column " << token.column;
    throw std::runtime_error(oss.str());
}

void Parser::Parse()
{
    tokens = lexer.Tokenize();
    currentTokenIndex = 0;
    m_AstRoot.reset();
    m_module_map.clear();
    if (is_end()) {
        throw std::runtime_error("Input is empty");
    }
    Module module;
    parse_module(module);
    m_module_map[module.modname] = module;
    if (!is_end()) {
        throw_parse_error("Unexpected trailing tokens");
    }
}

ModuleNode *Parser::parse_module(Module &module)
{
    if (!match_keyword("module")) {
        throw_parse_error("Expected 'module'");
    }
    module.modname = expect_identifier("Expected module name");
    m_AstRoot.reset(new ModuleNode(module.modname));
    expect_symbol("(");
    parse_module_header(module, *m_AstRoot);
    expect_symbol(")");
    expect_symbol(";");
    while (!(current_token().type == TokenType::Keyword && current_token().value == "endmodule")) {
        if (is_end()) {
            throw_parse_error("Expected 'endmodule'");
        }
        parse_module_item(module, *m_AstRoot);
    }
    consume();
    return m_AstRoot.get();
}

void Parser::parse_module_header(Module &module, ModuleNode &module_node)
{
    while (!(current_token().type == TokenType::Symbol && current_token().value == ")")) {
        if (current_token().type == TokenType::Keyword &&
            (current_token().value == "input" || current_token().value == "output" || current_token().value == "inout")) {
            parse_port_declaration(module, module_node, true);
        } else if (current_token().type == TokenType::Identifier) {
            for (;;) {
                std::string port_name = expect_identifier("Expected port name");
                module.add_port(port_name, PWire::PortType::None);
                module_node.update_port_direction(port_name, PWire::PortType::None);
                if (!match_symbol(",")) {
                    break;
                }
                if (current_token().type == TokenType::Keyword ||
                    (current_token().type == TokenType::Symbol && current_token().value == ")")) {
                    break;
                }
            }
        } else {
            throw_parse_error("Unsupported module header syntax");
        }
        if (current_token().type == TokenType::Symbol && current_token().value == ")") {
            break;
        }
        if (!(current_token().type == TokenType::Keyword &&
              (current_token().value == "input" || current_token().value == "output" || current_token().value == "inout"))) {
            expect_symbol(",");
        }
    }
}

void Parser::parse_module_item(Module &module, ModuleNode &module_node)
{
    if (current_token().type == TokenType::Keyword &&
        (current_token().value == "input" || current_token().value == "output" || current_token().value == "inout")) {
        parse_port_declaration(module, module_node, false);
        return;
    }
    if (current_token().type == TokenType::Keyword && current_token().value == "wire") {
        parse_wire_declaration(module, module_node);
        return;
    }
    if (current_token().type == TokenType::Keyword && current_token().value == "assign") {
        parse_assign_statement(module, module_node, true);
        return;
    }
    if (current_token().type == TokenType::Identifier && peek_token().type == TokenType::Symbol && peek_token().value == "=") {
        parse_assign_statement(module, module_node, false);
        return;
    }
    throw_parse_error("Unsupported module item");
}

void Parser::parse_port_declaration(Module &module, ModuleNode &module_node, bool in_header)
{
    PWire::PortType direction = parse_port_direction();
    if (current_token().type == TokenType::Keyword && current_token().value == "wire") {
        consume();
    }
    for (;;) {
        std::string name = expect_identifier("Expected port name");
        module.add_port(name, direction);
        module_node.update_port_direction(name, direction);
        if (!match_symbol(",")) {
            break;
        }
        if (in_header && (current_token().type == TokenType::Keyword ||
                          (current_token().type == TokenType::Symbol && current_token().value == ")"))) {
            break;
        }
    }
    if (!in_header) {
        expect_symbol(";");
    }
}

void Parser::parse_wire_declaration(Module &module, ModuleNode &module_node)
{
    if (!match_keyword("wire")) {
        throw_parse_error("Expected 'wire'");
    }
    for (;;) {
        std::string name = expect_identifier("Expected wire name");
        module.add_wire(name);
        module_node.add_statement(std::unique_ptr<AstNode>(new WireNode(name)));
        if (!match_symbol(",")) {
            break;
        }
    }
    expect_symbol(";");
}

void Parser::parse_assign_statement(Module &module, ModuleNode &module_node, bool explicit_assign_keyword)
{
    if (explicit_assign_keyword && !match_keyword("assign")) {
        throw_parse_error("Expected 'assign'");
    }
    std::string lhs = expect_identifier("Expected assignment target");
    if (!module.has_wire(lhs)) {
        throw_parse_error("Assignment target '" + lhs + "' is not declared");
    }
    expect_symbol("=");
    std::unique_ptr<ExprNode> rhs = parse_expression(module);
    expect_symbol(";");
    module_node.add_statement(std::unique_ptr<AstNode>(new AssignNode(lhs, std::move(rhs))));
}

std::unique_ptr<ExprNode> Parser::parse_expression(Module &module)
{
    return parse_conditional_expression(module);
}

std::unique_ptr<ExprNode> Parser::parse_conditional_expression(Module &module)
{
    std::unique_ptr<ExprNode> condition = parse_or_expression(module);
    if (match_symbol("?")) {
        std::unique_ptr<ExprNode> true_expr = parse_expression(module);
        expect_symbol(":");
        std::unique_ptr<ExprNode> false_expr = parse_conditional_expression(module);
        return std::unique_ptr<ExprNode>(
            new MuxExprNode(std::move(condition), std::move(true_expr), std::move(false_expr)));
    }
    return condition;
}

std::unique_ptr<ExprNode> Parser::parse_or_expression(Module &module)
{
    std::unique_ptr<ExprNode> left = parse_xor_expression(module);
    while (match_symbol("|")) {
        left.reset(new BinaryExprNode(std::move(left), '|', parse_xor_expression(module)));
    }
    return left;
}

std::unique_ptr<ExprNode> Parser::parse_xor_expression(Module &module)
{
    std::unique_ptr<ExprNode> left = parse_and_expression(module);
    while (match_symbol("^")) {
        left.reset(new BinaryExprNode(std::move(left), '^', parse_and_expression(module)));
    }
    return left;
}

std::unique_ptr<ExprNode> Parser::parse_and_expression(Module &module)
{
    std::unique_ptr<ExprNode> left = parse_unary_expression(module);
    while (match_symbol("&")) {
        left.reset(new BinaryExprNode(std::move(left), '&', parse_unary_expression(module)));
    }
    return left;
}

std::unique_ptr<ExprNode> Parser::parse_unary_expression(Module &module)
{
    if (match_symbol("~")) {
        return std::unique_ptr<ExprNode>(new UnaryExprNode('~', parse_unary_expression(module)));
    }
    return parse_primary_expression(module);
}

std::unique_ptr<ExprNode> Parser::parse_primary_expression(Module &module)
{
    if (match_symbol("(")) {
        std::unique_ptr<ExprNode> expr = parse_expression(module);
        expect_symbol(")");
        return expr;
    }
    if (current_token().type == TokenType::Identifier) {
        std::string name = current_token().value;
        if (!module.has_wire(name)) {
            throw_parse_error("Use of undeclared signal '" + name + "'");
        }
        consume();
        return std::unique_ptr<ExprNode>(new IdentifierExprNode(name));
    }
    if (current_token().type == TokenType::Number) {
        std::string value = current_token().value;
        consume();
        return std::unique_ptr<ExprNode>(new NumberExprNode(value));
    }
    throw_parse_error("Expected expression");
}

std::string ExprToString(const ExprNode *expr)
{
    if (expr == nullptr) return "";
    switch (expr->kind) {
    case ExprNode::Kind::Identifier:
        return static_cast<const IdentifierExprNode *>(expr)->name;
    case ExprNode::Kind::Number:
        return static_cast<const NumberExprNode *>(expr)->value;
    case ExprNode::Kind::Unary:
        return std::string(1, static_cast<const UnaryExprNode *>(expr)->op) +
               ExprToString(static_cast<const UnaryExprNode *>(expr)->operand.get());
    case ExprNode::Kind::Binary: {
        const BinaryExprNode *binary = static_cast<const BinaryExprNode *>(expr);
        return "(" + ExprToString(binary->left.get()) + " " + std::string(1, binary->op) + " " +
               ExprToString(binary->right.get()) + ")";
    }
    case ExprNode::Kind::Mux: {
        const MuxExprNode *mux = static_cast<const MuxExprNode *>(expr);
        return "(" + ExprToString(mux->condition.get()) + " ? " +
               ExprToString(mux->trueExpr.get()) + " : " +
               ExprToString(mux->falseExpr.get()) + ")";
    }
    default:
        return "";
    }
}
