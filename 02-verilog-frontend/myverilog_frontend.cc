#include "kernel/yosys.h"

#include "MyParser.h"
#include "Optimization.h"

#include <algorithm>
#include <fstream>
#include <map>
#include <sstream>

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

namespace {

RTLIL::IdString ToId(const std::string &name)
{
    return RTLIL::escape_id(name);
}

RTLIL::Const ParseNumberLiteral(const std::string &text)
{
    if (text == "1" || text == "1'b1" || text == "1'B1") return RTLIL::Const(1, 1);
    return RTLIL::Const(0, 1);
}

struct ExprBuilder {
    explicit ExprBuilder(RTLIL::Module *module) : module(module) {}

    RTLIL::SigSpec build(const ExprNode *expr)
    {
        if (expr->kind == ExprNode::Kind::Identifier) {
            const IdentifierExprNode *id = static_cast<const IdentifierExprNode *>(expr);
            RTLIL::Wire *wire = module->wire(ToId(id->name));
            if (wire == nullptr) log_error("Unknown signal '%s'.\n", id->name.c_str());
            return RTLIL::SigSpec(wire);
        }
        if (expr->kind == ExprNode::Kind::Number) {
            return RTLIL::SigSpec(ParseNumberLiteral(static_cast<const NumberExprNode *>(expr)->value));
        }
        if (expr->kind == ExprNode::Kind::Unary) {
            const UnaryExprNode *unary = static_cast<const UnaryExprNode *>(expr);
            RTLIL::SigSpec input = build(unary->operand.get());
            RTLIL::Wire *result = module->addWire(NEW_ID, GetSize(input));
            RTLIL::Cell *cell = module->addCell(NEW_ID, RTLIL::IdString("$not"));
            cell->setParam(ToId("A_SIGNED"), RTLIL::Const(0));
            cell->setParam(ToId("A_WIDTH"), RTLIL::Const(GetSize(input)));
            cell->setParam(ToId("Y_WIDTH"), RTLIL::Const(GetSize(input)));
            cell->setPort(ToId("A"), input);
            cell->setPort(ToId("Y"), RTLIL::SigSpec(result));
            return RTLIL::SigSpec(result);
        }
        if (expr->kind == ExprNode::Kind::Binary) {
            const BinaryExprNode *binary = static_cast<const BinaryExprNode *>(expr);
            RTLIL::SigSpec left = build(binary->left.get());
            RTLIL::SigSpec right = build(binary->right.get());
            int width = std::max(GetSize(left), GetSize(right));
            RTLIL::Wire *result = module->addWire(NEW_ID, width);
            std::string type = binary->op == '&' ? "$and" : binary->op == '|' ? "$or" : "$xor";
            RTLIL::Cell *cell = module->addCell(NEW_ID, RTLIL::IdString(type));
            cell->setParam(ToId("A_SIGNED"), RTLIL::Const(0));
            cell->setParam(ToId("B_SIGNED"), RTLIL::Const(0));
            cell->setParam(ToId("A_WIDTH"), RTLIL::Const(GetSize(left)));
            cell->setParam(ToId("B_WIDTH"), RTLIL::Const(GetSize(right)));
            cell->setParam(ToId("Y_WIDTH"), RTLIL::Const(width));
            cell->setPort(ToId("A"), left);
            cell->setPort(ToId("B"), right);
            cell->setPort(ToId("Y"), RTLIL::SigSpec(result));
            return RTLIL::SigSpec(result);
        }
        const MuxExprNode *mux = static_cast<const MuxExprNode *>(expr);
        RTLIL::SigSpec select = build(mux->condition.get());
        RTLIL::SigSpec true_value = build(mux->trueExpr.get());
        RTLIL::SigSpec false_value = build(mux->falseExpr.get());
        int width = std::max(GetSize(true_value), GetSize(false_value));
        RTLIL::Wire *result = module->addWire(NEW_ID, width);
        RTLIL::Cell *cell = module->addCell(NEW_ID, RTLIL::IdString("$mux"));
        cell->setParam(ToId("WIDTH"), RTLIL::Const(width));
        cell->setPort(ToId("A"), false_value);
        cell->setPort(ToId("B"), true_value);
        cell->setPort(ToId("S"), select);
        cell->setPort(ToId("Y"), RTLIL::SigSpec(result));
        return RTLIL::SigSpec(result);
    }

    RTLIL::Module *module;
};

void ConvertAstToRtlil(std::unique_ptr<ModuleNode> &ast_root,
                       RTLIL::Design *design,
                       const std::map<std::string, ::Module> &module_map)
{
    const ::Module &parsed_module = module_map.find(ast_root->name)->second;
    RTLIL::Module *rtlil_module = design->addModule(ToId(ast_root->name));
    int port_id = 1;
    for (size_t i = 0; i < parsed_module.port_order.size(); ++i) {
        const std::string &port_name = parsed_module.port_order[i];
        const PWire *wire_info = parsed_module.find_wire(port_name);
        RTLIL::Wire *wire = rtlil_module->addWire(ToId(port_name), 1);
        wire->port_id = port_id++;
        if (wire_info != nullptr && wire_info->porttype == PWire::PortType::Input) wire->port_input = true;
        if (wire_info != nullptr && wire_info->porttype == PWire::PortType::Output) wire->port_output = true;
    }
    for (std::map<std::string, PWire>::const_iterator it = parsed_module.wires.begin(); it != parsed_module.wires.end(); ++it) {
        if (it->second.wiretype == PWire::WireType::Internal) rtlil_module->addWire(ToId(it->first), 1);
    }
    ExprBuilder builder(rtlil_module);
    for (size_t i = 0; i < ast_root->statements.size(); ++i) {
        AssignNode *assign = dynamic_cast<AssignNode *>(ast_root->statements[i].get());
        if (assign == nullptr) continue;
        RTLIL::Wire *lhs = rtlil_module->wire(ToId(assign->lhs));
        rtlil_module->connect(RTLIL::SigSpec(lhs), builder.build(assign->rhsExpr.get()));
    }
    rtlil_module->fixup_ports();
}

} // namespace

struct MyVerilogFrontend : public Frontend {
    MyVerilogFrontend() : Frontend("myverilog", "parse a simple Verilog subset") {}

    void help() override
    {
        log("\n    read_myverilog [filename]\n\n");
        log("Reads a small Verilog subset and creates RTLIL.\n\n");
    }

    void execute(std::istream *&f, std::string filename, std::vector<std::string> args, RTLIL::Design *design) override
    {
        log_header(design, "Executing MYVERILOG frontend.\n");
        extra_args(f, filename, args, 1, design);
        std::ifstream file(filename.c_str(), std::ios::in | std::ios::binary);
        if (!file.is_open()) log_error("Could not open file: %s\n", filename.c_str());
        std::stringstream buffer;
        buffer << file.rdbuf();
        Lexer lexer(buffer.str());
        Parser parser(lexer);
        try {
            parser.Parse();
            OptimizeModuleAst(parser.m_AstRoot.get());
        } catch (const std::exception &e) {
            log_error("Parse error in %s: %s\n", filename.c_str(), e.what());
        }
        ConvertAstToRtlil(parser.m_AstRoot, design, parser.m_module_map);
    }
};

PRIVATE_NAMESPACE_END

YOSYS_NAMESPACE_BEGIN
Frontend *myverilog_frontend = new MyVerilogFrontend;
YOSYS_NAMESPACE_END
