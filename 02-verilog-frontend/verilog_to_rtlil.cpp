#include "MyParser.h"
#include "Optimization.h"

#include <fstream>
#include <iostream>
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

std::string Id(const std::string &name) { return "\\" + name; }

void WriteExprAsComment(const ExprNode *expr, std::ostream &out)
{
    out << "    # expr " << ExprToString(expr) << "\n";
}

void GenerateRtlil(std::unique_ptr<ModuleNode> &ast_root, const std::map<std::string, Module> &module_map, std::ostream &out)
{
    const Module &module = module_map.find(ast_root->name)->second;
    out << "autoidx 1\n";
    out << "module " << Id(ast_root->name) << "\n";
    for (size_t i = 0; i < module.port_order.size(); ++i) {
        const std::string &port_name = module.port_order[i];
        const PWire *wire = module.find_wire(port_name);
        out << "  wire width 1";
        if (wire != nullptr && wire->porttype == PWire::PortType::Input) out << " input " << (i + 1);
        if (wire != nullptr && wire->porttype == PWire::PortType::Output) out << " output " << (i + 1);
        out << " " << Id(port_name) << "\n";
    }
    for (std::map<std::string, PWire>::const_iterator it = module.wires.begin(); it != module.wires.end(); ++it) {
        if (it->second.wiretype == PWire::WireType::Internal) out << "  wire width 1 " << Id(it->first) << "\n";
    }
    for (size_t i = 0; i < ast_root->statements.size(); ++i) {
        AssignNode *assign = dynamic_cast<AssignNode *>(ast_root->statements[i].get());
        if (assign == nullptr) continue;
        WriteExprAsComment(assign->rhsExpr.get(), out);
        out << "  connect " << Id(assign->lhs) << " " << ExprToString(assign->rhsExpr.get()) << "\n";
    }
    out << "end\n";
}

} // namespace

int main(int argc, char *argv[])
{
    if (argc != 3) {
        std::cerr << "Usage: verilog_to_rtlil <input.v> <output.rtlil>" << std::endl;
        return 1;
    }
    try {
        Lexer lexer(ReadFile(argv[1]));
        Parser parser(lexer);
        parser.Parse();
        OptimizeModuleAst(parser.m_AstRoot.get());
        std::ofstream out(argv[2], std::ios::out | std::ios::trunc);
        GenerateRtlil(parser.m_AstRoot, parser.m_module_map, out);
        std::cout << "RTLIL generated successfully: " << argv[2] << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
