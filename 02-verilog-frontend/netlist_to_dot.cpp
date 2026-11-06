#include "MyParser.h"
#include "Optimization.h"

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {

std::string ReadFile(const std::string &filename)
{
    std::ifstream file(filename.c_str(), std::ios::in | std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("Failed to open file: " + filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string EscapeDot(const std::string &text)
{
    std::string escaped;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '"' || text[i] == '\\') escaped.push_back('\\');
        escaped.push_back(text[i]);
    }
    return escaped;
}

struct DotBuilder {
    DotBuilder(std::ostream &out, bool enable_cse) : out(out), gate_index(0), const_index(0), enable_cse(enable_cse) {}

    void Begin(const ModuleNode &module)
    {
        out << "digraph \"" << EscapeDot(module.name) << "\" {\n";
        out << "  rankdir=LR;\n";
        out << "  graph [fontname=\"Arial\"];\n";
        out << "  node [fontname=\"Arial\"];\n";
        out << "  edge [fontname=\"Arial\"];\n";
    }
    void End() { out << "}\n"; }

    void EmitSignal(const std::string &name, const std::string &shape)
    {
        if (signal_nodes.find(name) != signal_nodes.end()) return;
        signal_nodes[name] = name;
        out << "  \"" << EscapeDot(name) << "\" [shape=" << shape << "];\n";
    }

    std::string BuildExpr(const ExprNode *expr)
    {
        if (enable_cse && (expr->kind == ExprNode::Kind::Unary || expr->kind == ExprNode::Kind::Binary || expr->kind == ExprNode::Kind::Mux)) {
            std::map<std::string, std::string>::const_iterator it = expr_nodes.find(ExprKey(expr));
            if (it != expr_nodes.end()) return it->second;
        }
        if (expr->kind == ExprNode::Kind::Identifier) {
            std::string name = static_cast<const IdentifierExprNode *>(expr)->name;
            EmitSignal(name, "ellipse");
            return name;
        }
        if (expr->kind == ExprNode::Kind::Number) {
            std::ostringstream node;
            node << "const$" << ++const_index;
            out << "  \"" << node.str() << "\" [shape=plaintext,label=\"" << EscapeDot(static_cast<const NumberExprNode *>(expr)->value) << "\"];\n";
            return node.str();
        }
        if (expr->kind == ExprNode::Kind::Unary) {
            std::string input = BuildExpr(static_cast<const UnaryExprNode *>(expr)->operand.get());
            std::string gate = NewGate("NOT");
            out << "  \"" << EscapeDot(input) << "\" -> \"" << gate << "\";\n";
            CacheExpr(expr, gate);
            return gate;
        }
        if (expr->kind == ExprNode::Kind::Binary) {
            const BinaryExprNode *binary = static_cast<const BinaryExprNode *>(expr);
            std::string left = BuildExpr(binary->left.get());
            std::string right = BuildExpr(binary->right.get());
            std::string label = binary->op == '&' ? "AND" : binary->op == '|' ? "OR" : "XOR";
            std::string gate = NewGate(label);
            out << "  \"" << EscapeDot(left) << "\" -> \"" << gate << "\";\n";
            out << "  \"" << EscapeDot(right) << "\" -> \"" << gate << "\";\n";
            CacheExpr(expr, gate);
            return gate;
        }
        const MuxExprNode *mux = static_cast<const MuxExprNode *>(expr);
        std::string condition = BuildExpr(mux->condition.get());
        std::string true_value = BuildExpr(mux->trueExpr.get());
        std::string false_value = BuildExpr(mux->falseExpr.get());
        std::string gate = NewGate("MUX");
        out << "  \"" << EscapeDot(condition) << "\" -> \"" << gate << "\" [label=\"S\"];\n";
        out << "  \"" << EscapeDot(true_value) << "\" -> \"" << gate << "\" [label=\"1\"];\n";
        out << "  \"" << EscapeDot(false_value) << "\" -> \"" << gate << "\" [label=\"0\"];\n";
        CacheExpr(expr, gate);
        return gate;
    }

    void EmitAssign(const AssignNode &assign)
    {
        EmitSignal(assign.lhs, "ellipse");
        std::string rhs = BuildExpr(assign.rhsExpr.get());
        out << "  \"" << EscapeDot(rhs) << "\" -> \"" << EscapeDot(assign.lhs) << "\";\n";
    }

    std::string NewGate(const std::string &label)
    {
        std::ostringstream node;
        node << "gate$" << ++gate_index;
        out << "  \"" << node.str() << "\" [shape=box,label=\"" << label << "\"];\n";
        return node.str();
    }
    void CacheExpr(const ExprNode *expr, const std::string &node_name)
    {
        if (enable_cse) expr_nodes[ExprKey(expr)] = node_name;
    }

    std::ostream &out;
    int gate_index;
    int const_index;
    bool enable_cse;
    std::map<std::string, std::string> signal_nodes;
    std::map<std::string, std::string> expr_nodes;
};

} // namespace

int main(int argc, char *argv[])
{
    if (argc != 3 && argc != 4) {
        std::cerr << "Usage: netlist_to_dot [--no-opt] <input.v> <output.dot>" << std::endl;
        return 1;
    }
    try {
        bool optimize = true;
        int input_index = 1;
        if (argc == 4) {
            if (std::string(argv[1]) != "--no-opt") throw std::runtime_error("Unknown option");
            optimize = false;
            input_index = 2;
        }
        Lexer lexer(ReadFile(argv[input_index]));
        Parser parser(lexer);
        parser.Parse();
        if (optimize) OptimizeModuleAst(parser.m_AstRoot.get());
        std::map<std::string, Module>::const_iterator module_it = parser.m_module_map.find(parser.m_AstRoot->name);
        std::ofstream out(argv[input_index + 1], std::ios::out | std::ios::trunc);
        DotBuilder builder(out, optimize);
        builder.Begin(*parser.m_AstRoot);
        for (size_t i = 0; i < module_it->second.port_order.size(); ++i) {
            const std::string &port = module_it->second.port_order[i];
            const PWire *wire = module_it->second.find_wire(port);
            builder.EmitSignal(port, wire != nullptr && wire->porttype == PWire::PortType::Output ? "doublecircle" : "oval");
        }
        for (size_t i = 0; i < parser.m_AstRoot->statements.size(); ++i) {
            const AssignNode *assign = dynamic_cast<const AssignNode *>(parser.m_AstRoot->statements[i].get());
            if (assign != nullptr) builder.EmitAssign(*assign);
        }
        builder.End();
        std::cout << "DOT generated successfully: " << argv[input_index + 1] << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
