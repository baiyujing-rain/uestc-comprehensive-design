#pragma once

#ifndef MYPARSER_H
#define MYPARSER_H

#include "MyModule.h"

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

class AstNode {
public:
    virtual ~AstNode() {}
};

class ExprNode : public AstNode {
public:
    enum class Kind { Identifier, Number, Unary, Binary, Mux };
    explicit ExprNode(Kind kind) : kind(kind) {}
    virtual ~ExprNode() {}
    Kind kind;
};

class IdentifierExprNode : public ExprNode {
public:
    explicit IdentifierExprNode(const std::string &name);
    std::string name;
};

class NumberExprNode : public ExprNode {
public:
    explicit NumberExprNode(const std::string &value);
    std::string value;
};

class UnaryExprNode : public ExprNode {
public:
    UnaryExprNode(char op, std::unique_ptr<ExprNode> operand);
    char op;
    std::unique_ptr<ExprNode> operand;
};

class BinaryExprNode : public ExprNode {
public:
    BinaryExprNode(std::unique_ptr<ExprNode> left, char op, std::unique_ptr<ExprNode> right);
    std::unique_ptr<ExprNode> left;
    char op;
    std::unique_ptr<ExprNode> right;
};

class MuxExprNode : public ExprNode {
public:
    MuxExprNode(std::unique_ptr<ExprNode> condition,
                std::unique_ptr<ExprNode> true_expr,
                std::unique_ptr<ExprNode> false_expr);
    std::unique_ptr<ExprNode> condition;
    std::unique_ptr<ExprNode> trueExpr;
    std::unique_ptr<ExprNode> falseExpr;
};

class PortNode : public AstNode {
public:
    PortNode(const std::string &name, PWire::PortType direction);
    std::string name;
    PWire::PortType direction;
};

class WireNode : public AstNode {
public:
    explicit WireNode(const std::string &name);
    std::string name;
};

class AssignNode : public AstNode {
public:
    AssignNode(const std::string &lhs, std::unique_ptr<ExprNode> rhs_expr);
    std::string lhs;
    std::unique_ptr<ExprNode> rhsExpr;
};

class ModuleNode : public AstNode {
public:
    explicit ModuleNode(const std::string &name);
    void add_port(std::unique_ptr<PortNode> port);
    void add_statement(std::unique_ptr<AstNode> statement);
    void update_port_direction(const std::string &name, PWire::PortType direction);
    std::string name;
    std::vector<std::unique_ptr<PortNode> > ports;
    std::vector<std::unique_ptr<AstNode> > statements;
};

enum class TokenType { Identifier, Number, Keyword, Symbol, EndOfInput };

struct Token {
    Token();
    Token(TokenType type, const std::string &value, size_t line, size_t column);
    TokenType type;
    std::string value;
    size_t line;
    size_t column;
};

class Lexer {
public:
    explicit Lexer(const std::string &input);
    Token NextToken();
    std::vector<Token> Tokenize();

private:
    void skip_whitespace_and_comments();
    Token read_identifier_or_keyword();
    Token read_number();
    char peek(size_t offset = 0) const;
    char get();
    bool is_keyword(const std::string &value) const;

    std::string input;
    size_t position;
    size_t line;
    size_t column;
    std::unordered_set<std::string> keywords;
};

class Parser {
public:
    explicit Parser(Lexer &lexer);
    void Parse();

    std::unique_ptr<ModuleNode> m_AstRoot;
    std::map<std::string, Module> m_module_map;

private:
    ModuleNode *parse_module(Module &module);
    void parse_module_header(Module &module, ModuleNode &module_node);
    void parse_module_item(Module &module, ModuleNode &module_node);
    void parse_port_declaration(Module &module, ModuleNode &module_node, bool in_header);
    void parse_wire_declaration(Module &module, ModuleNode &module_node);
    void parse_assign_statement(Module &module, ModuleNode &module_node, bool explicit_assign_keyword);
    std::unique_ptr<ExprNode> parse_expression(Module &module);
    std::unique_ptr<ExprNode> parse_conditional_expression(Module &module);
    std::unique_ptr<ExprNode> parse_or_expression(Module &module);
    std::unique_ptr<ExprNode> parse_xor_expression(Module &module);
    std::unique_ptr<ExprNode> parse_and_expression(Module &module);
    std::unique_ptr<ExprNode> parse_unary_expression(Module &module);
    std::unique_ptr<ExprNode> parse_primary_expression(Module &module);
    const Token &current_token() const;
    const Token &peek_token(size_t offset = 1) const;
    bool is_end() const;
    bool match_symbol(const std::string &value);
    bool match_keyword(const std::string &value);
    void expect_symbol(const std::string &value);
    std::string expect_identifier(const std::string &message);
    PWire::PortType parse_port_direction();
    void consume();
    [[noreturn]] void throw_parse_error(const std::string &message) const;

    Lexer &lexer;
    std::vector<Token> tokens;
    size_t currentTokenIndex;
};

std::string ExprToString(const ExprNode *expr);

#endif
