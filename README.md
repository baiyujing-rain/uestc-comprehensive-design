# 电子科技大学《综合设计》课程项目合集

本仓库收录三个学期「工业软件创新训练 / 进阶式挑战性综合项目」综合设计的完整源码：

| 目录 | 学期 | 项目 | 技术栈 | 课程文档 |
| --- | --- | --- | --- | --- |
| [`01-circuit-workbench`](01-circuit-workbench) | 综合设计 I | 电路工作台：数字电路可视化编辑、连线与稳态仿真 | C++17 / wxWidgets / MSVC | 课题任务书 |
| [`02-verilog-frontend`](02-verilog-frontend) | 综合设计 II | Verilog 前端：语法解析、AST 优化、DOT 网表、RTLIL 导出与 Yosys 插件 | C++11 / Yosys / Graphviz | 任务描述 |
| [`03-myplacement`](03-myplacement) | 综合设计 III | MyPlacement：芯片布局（Placement）工具链 | C++14 / CMake / Eigen / CImg / Flask | 任务描述 |

---

## 项目速览

### 01 · 电路工作台（综合设计 I）

基于 wxWidgets 的桌面级数字电路编辑器：元件库拖放、正交折线连线、全加器示例、组合逻辑稳态仿真、`.circuit` / `.net` 文件读写与 SPICE 导出。已通过 63 项回归检查，记录见 `regression.txt`。

- 直接运行：`bin/CircuitWorkbench.exe`（随仓库附带，Windows 可直接双击）
- 自行构建：`build.ps1`（需 Visual Studio 2022 + wxWidgets 3.2.2.1 x64 Debug 静态库）
- 详细说明：[01-circuit-workbench/README.md](01-circuit-workbench/README.md)

### 02 · Verilog 前端与逻辑优化（综合设计 II）

从零实现的 Verilog 子集编译器前端：手写词法/语法分析生成 AST，`opt_muxtree` / `opt_expr` 风格的代数优化，输出 Graphviz DOT 网表与 RTLIL，并提供 Yosys 前端插件与本地可视化面板。

- 构建：`make all`（g++ / MinGW），Yosys 插件在 Ubuntu 下 `make plugin`
- 示例电路：`test.v`、`difficulty1~3.v` 及三个优化验证用例
- 详细说明：[02-verilog-frontend/README.md](02-verilog-frontend/README.md)

### 03 · MyPlacement 布局工具链（综合设计 III）

完整 EDA 布局（Placement）流水线：解析 UCLA BookShelf 格式电路 → 三种初始布局（随机 / 聚类 / 二次方程）→ ePlace 全局布局（WA 线长 + 静电密度 + Nesterov）→ BMP / GDSII 可视化 → Abacus 合法化，附带 Flask + 单页前端。

- 构建：`cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build`（Linux / WSL）
- 示例数据：`data/thin1`（玩具电路）、`data/adaptec1`（21 万单元真实基准）
- 详细说明：[03-myplacement/README.md](03-myplacement/README.md)

---

## 文档说明

本仓库**不收录课程报告**（中期报告、总结报告、汇报 PPT 等）：这些文件包含小组成员姓名、学号与指导教师联系方式等个人信息，不随源码公开。

只收录不含个人信息的课程任务文档：

| 项目 | 文档 |
| --- | --- |
| 综合设计 I | [`01-circuit-workbench/docs/课题任务书.docx`](01-circuit-workbench/docs/课题任务书.docx) |
| 综合设计 II | [`02-verilog-frontend/docs/任务描述.pdf`](02-verilog-frontend/docs/任务描述.pdf) |
| 综合设计 III | [`03-myplacement/docs/任务描述.pdf`](03-myplacement/docs/任务描述.pdf) |

综合设计 II、III 的《课题任务书》写有指导教师姓名、电话与邮箱，未收录。

---

## 仓库约定

- 三个项目各自保留原来的目录结构与构建脚本，未做重排，克隆后可直接按各自 README 构建。
- 不包含任何构建产物（`.vs/`、`x64/`、`build/`、`output/`）、本机配置（`*.user`）与运行产物（BMP / GDS / 日志）。
- 例外：`01-circuit-workbench/bin/CircuitWorkbench.exe` 与 `02-verilog-frontend` 下三个 `.exe` 是为方便直接演示而保留的预编译 Windows 程序，源码可用 `build.ps1` / `make` 重新生成。
