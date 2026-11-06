# Verilog 前端与逻辑优化工具 · 综合设计 II

电子科技大学《进阶式挑战性综合项目 II》第四部分「Verilog 编译器」课程项目（第 8 组）。

从零实现的小型 Verilog 前端工具链：

- 手写词法 / 语法分析，把 Verilog 子集解析为 AST
- 基于 AST 的逻辑优化（等价于 Yosys 的 `opt_muxtree` / `opt_expr`：多路选择器简化、常量折叠、表达式重写）
- 导出 Graphviz DOT 网表（优化前 / 优化后对比）与 RTLIL
- 可作为 **Yosys 前端插件**（`read_myverilog`）接入完整综合流程
- 附带本地可视化面板 `ui_dashboard.html`

课程任务文档见 [`docs/任务描述.pdf`](docs/任务描述.pdf)（任务一～五说明）；课程报告含个人信息，未随仓库发布。

## 构建

```bash
# 三个命令行工具（g++ / MinGW / MSVC 均可）
make all          # 生成 verilog_to_rtlil.exe、myverilog_parser.exe、netlist_to_dot.exe

# Yosys 前端插件（Ubuntu）
make plugin       # 生成 myverilog_frontend.so
```

`Makefile` 默认 `g++ -std=c++11`；插件目标使用 `yosys-config`，需要先安装 Yosys 开发包。

## 使用

```bash
# 1. 打印语法树
./myverilog_parser.exe test.v

# 2. 生成优化前 / 优化后的 DOT 网表
./netlist_to_dot.exe --no-opt test.v test_before.dot
./netlist_to_dot.exe          test.v test_after.dot

# 3. DOT 转 PNG（需要 Graphviz）
dot -Tpng test_before.dot -o test_before.png

# 4. 导出 RTLIL
./verilog_to_rtlil.exe test.v test.rtlil

# 5. Ubuntu 下作为 Yosys 自定义前端
yosys -m ./myverilog_frontend.so -p "read_myverilog test.v; opt_muxtree; opt_expr; opt; stat"
```

完整命令清单见 [`commands.txt`](commands.txt)。

## 示例电路

| 文件 | 说明 |
| --- | --- |
| `test.v` | 基础组合逻辑样例 |
| `difficulty1.v` / `difficulty2.v` / `difficulty3.v` | 由易到难的三组测试电路 |
| `mux_optimization_test.v` | 多路选择器优化用例 |
| `expr_optimization_test.v` | 表达式优化用例 |
| `rewrite_optimization_test.v` | 重写规则优化用例 |
| `ui_assets/` | 上述用例的优化前后 DOT / PNG 对照结果 |

## 源码结构

| 文件 | 作用 |
| --- | --- |
| `MyParser.h/.cpp` | 词法分析、语法分析、AST 构建与输出 |
| `MyModule.h/.cpp` | 模块、信号、赋值语句等语法对象的定义 |
| `Optimization.h/.cpp` | AST 层面的逻辑优化：mux 简化、常量折叠、表达式重写 |
| `main.cpp` | `myverilog_parser`：解析并打印 AST |
| `netlist_to_dot.cpp` | `netlist_to_dot`：生成优化前 / 后的 DOT 网表 |
| `verilog_to_rtlil.cpp` | `verilog_to_rtlil`：导出 RTLIL |
| `myverilog_frontend.cc` | Yosys 前端插件，注册 `read_myverilog` 命令 |
| `ui_dashboard.html` | 本地可视化面板（展示优化前后网表与截图） |

仓库内附带的三个 `.exe` 为 Windows 预编译版本，方便直接演示；可用 `make all` 重新生成。
