#include "Optimization.h"

#include <algorithm>
#include <map>
#include <memory>

namespace {

bool TryGetOneBitConstant(const ExprNode *expr, char &value)
{
    if (expr == nullptr || expr->kind != ExprNode::Kind::Number) return false;
    const std::string &text = static_cast<const NumberExprNode *>(expr)->value;
    if (text == "0" || text == "1") {
        value = text[0];
        return true;
    }
    if (text == "1'b0" || text == "1'B0") {
        value = '0';
        return true;
    }
    if (text == "1'b1" || text == "1'B1") {
        value = '1';
        return true;
    }
    return false;
}

std::unique_ptr<ExprNode> CloneExpr(const ExprNode *expr)
{
    if (expr == nullptr) return std::unique_ptr<ExprNode>();
    switch (expr->kind) {
    case ExprNode::Kind::Identifier:
        return std::unique_ptr<ExprNode>(new IdentifierExprNode(static_cast<const IdentifierExprNode *>(expr)->name));
    case ExprNode::Kind::Number:
        return std::unique_ptr<ExprNode>(new NumberExprNode(static_cast<const NumberExprNode *>(expr)->value));
    case ExprNode::Kind::Unary: {
        const UnaryExprNode *unary = static_cast<const UnaryExprNode *>(expr);
        return std::unique_ptr<ExprNode>(new UnaryExprNode(unary->op, CloneExpr(unary->operand.get())));
    }
    case ExprNode::Kind::Binary: {
        const BinaryExprNode *binary = static_cast<const BinaryExprNode *>(expr);
        return std::unique_ptr<ExprNode>(new BinaryExprNode(CloneExpr(binary->left.get()), binary->op, CloneExpr(binary->right.get())));
    }
    case ExprNode::Kind::Mux: {
        const MuxExprNode *mux = static_cast<const MuxExprNode *>(expr);
        return std::unique_ptr<ExprNode>(new MuxExprNode(CloneExpr(mux->condition.get()), CloneExpr(mux->trueExpr.get()), CloneExpr(mux->falseExpr.get())));
    }
    default:
        return std::unique_ptr<ExprNode>();
    }
}

std::unique_ptr<ExprNode> MakeOneBitConstant(char value)
{
    return std::unique_ptr<ExprNode>(new NumberExprNode(value == '0' ? "1'b0" : "1'b1"));
}

std::unique_ptr<ExprNode> MakeNot(std::unique_ptr<ExprNode> operand)
{
    return std::unique_ptr<ExprNode>(new UnaryExprNode('~', std::move(operand)));
}

std::unique_ptr<ExprNode> MakeBinary(std::unique_ptr<ExprNode> left, char op, std::unique_ptr<ExprNode> right)
{
    return std::unique_ptr<ExprNode>(new BinaryExprNode(std::move(left), op, std::move(right)));
}

bool ExprSame(const ExprNode *left, const ExprNode *right)
{
    return ExprKey(left) == ExprKey(right);
}

const BinaryExprNode *AsBinaryWithOp(const ExprNode *expr, char op)
{
    if (expr == nullptr || expr->kind != ExprNode::Kind::Binary) return nullptr;
    const BinaryExprNode *binary = static_cast<const BinaryExprNode *>(expr);
    return binary->op == op ? binary : nullptr;
}

bool BinaryContains(const BinaryExprNode *binary, const ExprNode *needle)
{
    return binary != nullptr && (ExprSame(binary->left.get(), needle) || ExprSame(binary->right.get(), needle));
}

std::unique_ptr<ExprNode> OptExpr(std::unique_ptr<ExprNode> expr, const std::map<std::string, std::string> &known_constants);

std::unique_ptr<ExprNode> OptMuxTree(std::unique_ptr<ExprNode> expr)
{
    if (expr == nullptr || expr->kind != ExprNode::Kind::Mux) return expr;
    MuxExprNode *mux = static_cast<MuxExprNode *>(expr.get());
    char condition_value = '\0';
    if (TryGetOneBitConstant(mux->condition.get(), condition_value)) {
        return condition_value == '1' ? std::move(mux->trueExpr) : std::move(mux->falseExpr);
    }
    if (ExprSame(mux->trueExpr.get(), mux->falseExpr.get())) {
        return std::move(mux->trueExpr);
    }
    char true_value = '\0', false_value = '\0';
    bool true_is_const = TryGetOneBitConstant(mux->trueExpr.get(), true_value);
    bool false_is_const = TryGetOneBitConstant(mux->falseExpr.get(), false_value);
    if (true_is_const && false_is_const) {
        if (true_value == '1' && false_value == '0') return std::move(mux->condition);
        if (true_value == '0' && false_value == '1') return MakeNot(std::move(mux->condition));
    }
    if (mux->trueExpr->kind == ExprNode::Kind::Mux) {
        MuxExprNode *true_mux = static_cast<MuxExprNode *>(mux->trueExpr.get());
        if (ExprSame(mux->condition.get(), true_mux->condition.get())) {
            return std::unique_ptr<ExprNode>(new MuxExprNode(std::move(mux->condition), std::move(true_mux->trueExpr), std::move(mux->falseExpr)));
        }
    }
    if (mux->falseExpr->kind == ExprNode::Kind::Mux) {
        MuxExprNode *false_mux = static_cast<MuxExprNode *>(mux->falseExpr.get());
        if (ExprSame(mux->condition.get(), false_mux->condition.get())) {
            return std::unique_ptr<ExprNode>(new MuxExprNode(std::move(mux->condition), std::move(mux->trueExpr), std::move(false_mux->falseExpr)));
        }
    }
    return expr;
}

std::unique_ptr<ExprNode> OptRewrite(std::unique_ptr<ExprNode> expr)
{
    if (expr == nullptr) return expr;
    if (expr->kind == ExprNode::Kind::Unary) {
        UnaryExprNode *unary = static_cast<UnaryExprNode *>(expr.get());
        if (unary->op == '~' && unary->operand->kind == ExprNode::Kind::Unary) {
            UnaryExprNode *nested = static_cast<UnaryExprNode *>(unary->operand.get());
            if (nested->op == '~') return std::move(nested->operand);
        }
        if (unary->op == '~' && unary->operand->kind == ExprNode::Kind::Binary) {
            BinaryExprNode *binary = static_cast<BinaryExprNode *>(unary->operand.get());
            if (binary->op == '&') return MakeBinary(MakeNot(std::move(binary->left)), '|', MakeNot(std::move(binary->right)));
            if (binary->op == '|') return MakeBinary(MakeNot(std::move(binary->left)), '&', MakeNot(std::move(binary->right)));
        }
        return expr;
    }
    if (expr->kind != ExprNode::Kind::Binary) return expr;
    BinaryExprNode *binary = static_cast<BinaryExprNode *>(expr.get());
    if (binary->op == '|') {
        const BinaryExprNode *left_and = AsBinaryWithOp(binary->left.get(), '&');
        const BinaryExprNode *right_and = AsBinaryWithOp(binary->right.get(), '&');
        if (BinaryContains(right_and, binary->left.get())) return std::move(binary->left);
        if (BinaryContains(left_and, binary->right.get())) return std::move(binary->right);
        if (left_and != nullptr && right_and != nullptr) {
            if (ExprSame(left_and->left.get(), right_and->left.get())) return MakeBinary(CloneExpr(left_and->left.get()), '&', MakeBinary(CloneExpr(left_and->right.get()), '|', CloneExpr(right_and->right.get())));
            if (ExprSame(left_and->left.get(), right_and->right.get())) return MakeBinary(CloneExpr(left_and->left.get()), '&', MakeBinary(CloneExpr(left_and->right.get()), '|', CloneExpr(right_and->left.get())));
            if (ExprSame(left_and->right.get(), right_and->left.get())) return MakeBinary(CloneExpr(left_and->right.get()), '&', MakeBinary(CloneExpr(left_and->left.get()), '|', CloneExpr(right_and->right.get())));
            if (ExprSame(left_and->right.get(), right_and->right.get())) return MakeBinary(CloneExpr(left_and->right.get()), '&', MakeBinary(CloneExpr(left_and->left.get()), '|', CloneExpr(right_and->left.get())));
        }
    }
    if (binary->op == '&') {
        const BinaryExprNode *left_or = AsBinaryWithOp(binary->left.get(), '|');
        const BinaryExprNode *right_or = AsBinaryWithOp(binary->right.get(), '|');
        if (BinaryContains(right_or, binary->left.get())) return std::move(binary->left);
        if (BinaryContains(left_or, binary->right.get())) return std::move(binary->right);
    }
    return expr;
}

std::unique_ptr<ExprNode> OptExpr(std::unique_ptr<ExprNode> expr, const std::map<std::string, std::string> &known_constants)
{
    if (expr == nullptr) return expr;
    if (expr->kind == ExprNode::Kind::Identifier) {
        const IdentifierExprNode *id = static_cast<const IdentifierExprNode *>(expr.get());
        std::map<std::string, std::string>::const_iterator it = known_constants.find(id->name);
        if (it != known_constants.end()) return std::unique_ptr<ExprNode>(new NumberExprNode(it->second));
        return expr;
    }
    if (expr->kind == ExprNode::Kind::Unary) {
        UnaryExprNode *unary = static_cast<UnaryExprNode *>(expr.get());
        unary->operand = OptExpr(std::move(unary->operand), known_constants);
        char value = '\0';
        if (unary->op == '~' && TryGetOneBitConstant(unary->operand.get(), value)) return MakeOneBitConstant(value == '0' ? '1' : '0');
        return OptRewrite(std::move(expr));
    }
    if (expr->kind == ExprNode::Kind::Mux) {
        MuxExprNode *mux = static_cast<MuxExprNode *>(expr.get());
        mux->condition = OptExpr(std::move(mux->condition), known_constants);
        mux->trueExpr = OptExpr(std::move(mux->trueExpr), known_constants);
        mux->falseExpr = OptExpr(std::move(mux->falseExpr), known_constants);
        return OptMuxTree(std::move(expr));
    }
    if (expr->kind != ExprNode::Kind::Binary) return expr;
    BinaryExprNode *binary = static_cast<BinaryExprNode *>(expr.get());
    binary->left = OptExpr(std::move(binary->left), known_constants);
    binary->right = OptExpr(std::move(binary->right), known_constants);
    char lv = '\0', rv = '\0';
    bool lc = TryGetOneBitConstant(binary->left.get(), lv);
    bool rc = TryGetOneBitConstant(binary->right.get(), rv);
    if (lc && rc) {
        char result = '0';
        if (binary->op == '&') result = lv == '1' && rv == '1' ? '1' : '0';
        if (binary->op == '|') result = lv == '1' || rv == '1' ? '1' : '0';
        if (binary->op == '^') result = lv == rv ? '0' : '1';
        return MakeOneBitConstant(result);
    }
    if (binary->op == '&') {
        if ((lc && lv == '0') || (rc && rv == '0')) return MakeOneBitConstant('0');
        if (lc && lv == '1') return std::move(binary->right);
        if (rc && rv == '1') return std::move(binary->left);
    } else if (binary->op == '|') {
        if ((lc && lv == '1') || (rc && rv == '1')) return MakeOneBitConstant('1');
        if (lc && lv == '0') return std::move(binary->right);
        if (rc && rv == '0') return std::move(binary->left);
    } else if (binary->op == '^') {
        if (lc && lv == '0') return std::move(binary->right);
        if (rc && rv == '0') return std::move(binary->left);
        if (lc && lv == '1') return MakeNot(std::move(binary->right));
        if (rc && rv == '1') return MakeNot(std::move(binary->left));
    }
    if ((binary->op == '&' || binary->op == '|' || binary->op == '^') && ExprSame(binary->left.get(), binary->right.get())) {
        if (binary->op == '^') return MakeOneBitConstant('0');
        return std::move(binary->left);
    }
    return OptRewrite(std::move(expr));
}

std::unique_ptr<ExprNode> RunOptimizationPasses(std::unique_ptr<ExprNode> expr, const std::map<std::string, std::string> &known_constants)
{
    for (int i = 0; i < 8; ++i) {
        std::string before = ExprKey(expr.get());
        expr = OptExpr(std::move(expr), known_constants);
        if (before == ExprKey(expr.get())) break;
    }
    return expr;
}

} // namespace

std::string ExprKey(const ExprNode *expr)
{
    if (expr == nullptr) return "";
    switch (expr->kind) {
    case ExprNode::Kind::Identifier:
        return "id:" + static_cast<const IdentifierExprNode *>(expr)->name;
    case ExprNode::Kind::Number:
        return "num:" + static_cast<const NumberExprNode *>(expr)->value;
    case ExprNode::Kind::Unary:
        return std::string("unary:") + static_cast<const UnaryExprNode *>(expr)->op + ":" + ExprKey(static_cast<const UnaryExprNode *>(expr)->operand.get());
    case ExprNode::Kind::Binary: {
        const BinaryExprNode *binary = static_cast<const BinaryExprNode *>(expr);
        std::string left = ExprKey(binary->left.get());
        std::string right = ExprKey(binary->right.get());
        if ((binary->op == '&' || binary->op == '|' || binary->op == '^') && right < left) std::swap(left, right);
        return std::string("binary:") + binary->op + ":" + left + ":" + right;
    }
    case ExprNode::Kind::Mux: {
        const MuxExprNode *mux = static_cast<const MuxExprNode *>(expr);
        return "mux:" + ExprKey(mux->condition.get()) + ":" + ExprKey(mux->trueExpr.get()) + ":" + ExprKey(mux->falseExpr.get());
    }
    default:
        return "";
    }
}

void OptimizeModuleAst(ModuleNode *module)
{
    if (module == nullptr) return;
    std::map<std::string, std::string> known_constants;
    for (size_t i = 0; i < module->statements.size(); ++i) {
        AssignNode *assign = dynamic_cast<AssignNode *>(module->statements[i].get());
        if (assign == nullptr) continue;
        assign->rhsExpr = RunOptimizationPasses(std::move(assign->rhsExpr), known_constants);
        char value = '\0';
        if (TryGetOneBitConstant(assign->rhsExpr.get(), value)) {
            known_constants[assign->lhs] = value == '0' ? "1'b0" : "1'b1";
        } else {
            known_constants.erase(assign->lhs);
        }
    }
}
