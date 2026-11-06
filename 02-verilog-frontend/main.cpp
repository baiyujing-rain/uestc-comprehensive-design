#include "MyParser.h"
#include "Optimization.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {

void PrintIndent(int depth) { std::cout << std::string(depth * 2, ' '); }

void PrintExpr(const ExprNode *expr, int depth)
{
    if (expr == nullptr) return;
    switch (expr->kind) {
    case ExprNode::Kind::Identifier:
        PrintIndent(depth);
        std::cout << "Id: " << static_cast<const IdentifierExprNode *>(expr)->name << std::endl;
        return;
    case ExprNode::Kind::Number:
        PrintIndent(depth);
        std::cout << "Number: " << static_cast<const NumberExprNode *>(expr)->value << std::endl;
        return;
    case ExprNode::Kind::Unary: {
        const UnaryExprNode *unary = static_cast<const UnaryExprNode *>(expr);
        PrintIndent(depth);
        std::cout << "UnaryExpr: " << unary->op << std::endl;
        PrintExpr(unary->operand.get(), depth + 1);
        return;
    }
    case ExprNode::Kind::Binary: {
        const BinaryExprNode *binary = static_cast<const BinaryExprNode *>(expr);
        PrintIndent(depth);
        std::cout << "BinaryExpr: " << binary->op << std::endl;
        PrintExpr(binary->left.get(), depth + 1);
        PrintExpr(binary->right.get(), depth + 1);
        return;
    }
    case ExprNode::Kind::Mux: {
        const MuxExprNode *mux = static_cast<const MuxExprNode *>(expr);
        PrintIndent(depth);
        std::cout << "MuxExpr" << std::endl;
        PrintIndent(depth + 1);
        std::cout << "Condition:" << std::endl;
        PrintExpr(mux->condition.get(), depth + 2);
        PrintIndent(depth + 1);
        std::cout << "True:" << std::endl;
        PrintExpr(mux->trueExpr.get(), depth + 2);
        PrintIndent(depth + 1);
        std::cout << "False:" << std::endl;
        PrintExpr(mux->falseExpr.get(), depth + 2);
        return;
    }
    }
}

std::string PortDirectionToString(PWire::PortType direction)
{
    if (direction == PWire::PortType::Input) return "input";
    if (direction == PWire::PortType::Output) return "output";
    if (direction == PWire::PortType::Inout) return "inout";
    return "port";
}

std::string ReadFile(const std::string &filename)
{
    std::ifstream file(filename.c_str(), std::ios::in | std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("Failed to open file: " + filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

} // namespace

int main(int argc, char *argv[])
{
    try {
        std::string filename = argc > 1 ? argv[1] : "test.v";
        Lexer lexer(ReadFile(filename));
        Parser parser(lexer);
        parser.Parse();
        OptimizeModuleAst(parser.m_AstRoot.get());

        const ModuleNode *module = parser.m_AstRoot.get();
        std::cout << "Module: " << module->name << std::endl;
        std::cout << "Ports:" << std::endl;
        for (size_t i = 0; i < module->ports.size(); ++i) {
            std::cout << "  " << PortDirectionToString(module->ports[i]->direction) << ": " << module->ports[i]->name << std::endl;
        }
        std::cout << "Statements:" << std::endl;
        for (size_t i = 0; i < module->statements.size(); ++i) {
            const WireNode *wire = dynamic_cast<const WireNode *>(module->statements[i].get());
            if (wire != nullptr) {
                std::cout << "  Wire: " << wire->name << std::endl;
                continue;
            }
            const AssignNode *assign = dynamic_cast<const AssignNode *>(module->statements[i].get());
            if (assign != nullptr) {
                std::cout << "  Assign: " << assign->lhs << " = " << ExprToString(assign->rhsExpr.get()) << std::endl;
                PrintExpr(assign->rhsExpr.get(), 2);
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
