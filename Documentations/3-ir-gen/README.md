# Lab3 实验文档

- [Lab3 实验文档](#lab3-实验文档)
  - [0. 前言](#0-前言)
    - [主要工作](#主要工作)
  - [1. 实验框架](#1-实验框架)
  - [2. 运行与调试](#2-运行与调试)
    - [运行 cminusfc](#运行-cminusfc)
    - [自动测试](#自动测试)
    - [logging](#logging)
    - [建议](#建议)
  - [3. 提交要求](#3-提交要求)
    - [目录结构](#目录结构)
    - [提交要求和评分标准](#提交要求和评分标准)

## 0. 前言

本次实验是组队实验，请仔细阅读[组队要求](http://211.86.152.198:8080/staff/2021fall-notice_board/-/issues/46)，并合理进行分工合作。
本次实验和 Lab2 一样，需要使用 `LightIR` 框架自动产生 `cminus-f` 语言的LLVM IR。
建议在实验前队伍内进行讨论，以确保成员都理解 Lab2 中的问题。

经过 Lab2 的练手，相信大家已经掌握了 LightIR 的使用，并且对于 LLVM IR 也有了一定理解。在本次实验中，我们要使用访问者模式来实现 IR 自动生成。
对于产生的IR，我们可以调用 clang 生成可执行文件，这样一个初级的 cminus-f 编译器就完成啦！

### 主要工作

1. 阅读[cminus-f 的语义规则](../common/cminusf.md)，我们将按照语义实现程度进行评分
2. 阅读[LightIR 核心类介绍](../common/LightIR.md)
3. 阅读[实验框架](#1-实验框架)，理解如何使用框架以及注意事项
4. 修改 `src/cminusfc/cminusf_builder.cpp` 来实现自动 IR 产生的算法，使得它能正确编译任何合法的 cminus-f 程序
5. 在 `report.md` 中解释你们的设计，遇到的困难和解决方案
6. 由**队长**在 `contribution.md` 中解释每位队员的贡献，并说明贡献比例

**注意**：
1. 我们将对本次实验中提交的材料进行查重，抄袭行为（包括使用公开仓库的代码）都是不允许的。
2. 小组间的正常交流是允许的，但应当遵循独立性原则，严禁代码共享。如果与其它组进行了交流，必须在 `report.md` 中声明，并记录下来交流内容和信息流动的方向。

## 1. 实验框架

本次实验使用了由C++编写的 LightIR 来生成 LLVM IR。为了便于大家进行实验，该框架自动完成了语法树到 C++ 上的抽象语法树的转换。
我们可以使用[访问者模式](../2-ir-gen-warmup/README.md#31-%E4%BA%86%E8%A7%A3-visitor-pattern)来设计抽象语法树
中的算法。大家可以参考[lab2的样例](../../tests/2-ir-gen-warmup/calculator/calc_builder.cpp)和[打印抽象语法树的算法](../../src/common/ast.cpp#L394-737)，
以及运行 `test_ast` 来理解访问者模式下算法的执行流程。

在`include/cminusf_builder.hpp`中，我还定义了一个用于存储作用域的类`Scope`。它的作用是辅助我们在遍历语法树时，管理不同作用域中的变量。它提供了以下接口：
```cpp
// 进入一个新的作用域
void enter();
// 退出一个作用域
void exit();
// 往当前作用域插入新的名字->值映射
bool push(std::string name, Value *val);
// 根据名字，寻找到值
Value* find(std::string name);
// 判断当前是否在全局作用域内
bool in_global();
```
你们需要根据语义合理调用`enter`与`exit`，并且在变量声明和使用时正确调用`push`与`find`。在类`CminusfBuilder`中，有一个`Scope`类型的成员变量`scope`，它在初始化时已经将`input`、`output`等函数加入了作用域中。因此，你们在进行名字查找时不需要顾虑是否需要对特殊函数进行特殊操作。

## 2. 运行与调试

### 运行 cminusfc

```sh
mkdir build && cd build
make -j
# 安装以链接 libcminus_io.a
sudo make install
```

编译后会生成 `cminusfc` 程序，这就是最终的 cminus-f 编译器程序。它能编译 cminus-f 源程序文件并输出相应 LLVM IR，也可以再利用 clang 将 IR 编译成二进制。编译器的命令行相关逻辑实现在 `src/cminusfc/cminusfc.cpp` 中，自动代码生成逻辑写在`src/cminusfc/cminusf_builder.cpp`中。

当需要对 `.cminus` 文件测试时，可以这样使用：

```sh
# 假设 cminusfc 的路径在你的$PATH中
# 利用构建好的 Module 生成 test.ll
# 注意，如果调用了外部函数，如 input, output 等，则无法使用lli运行
cminusfc test.cminus -emit-llvm

# 假设libcminus_io.a的路径在$LD_LIBRARY_PATH中，clang的路径在$PATH中
# 1. 利用构建好的 Module 生成 test.ll
# 2. 调用 clang 来编译 IR 并链接上静态链接库 libcminus_io.a，生成二进制文件 test
cminusfc test.cminus
```

### 测试

自动测试脚本和所有测试样例都是公开的，它在 `tests/3-ir-gen` 目录下，使用方法如下：
```sh
# 在 tests/3-ir-gen 目录下运行：
./eval.py
```
测试结果会输出到同文件夹的 `eval_result` 下。

### logging

[logging](../common/logging.md) 是帮助大家打印调试信息的工具，如有需求可以阅读文档后进行使用

### 建议

1. 比较你们编写的编译器产生的 IR 和 clang 产生的IR来找出可能的问题或发现新的思路
2. 使用 logging 工具来打印调试信息
2. 使用 GDB 等软件进行单步调试来检查错误的原因
3. 合理分工

## 3. 提交要求

### 目录结构

```
.
├── CMakeLists.txt
├── Documentations
│   ├── ...
│   ├── common
│   |   ├── LightIR.md                  <- LightIR 相关文档
│   |   ├── logging.md                  <- logging 工具相关文档
│   |   └── cminusf.md                  <- cminus-f 的语法和语义文档
│   └── 3-ir-gen
│       └── README.md                   <- lab3 实验文档说明（你在这里）
├── include                             <- 实验所需的头文件
│   ├── ...
│   ├── lightir/*
│   ├── cminusf_builder.hpp
|   └── ast.hpp
├── Reports
│   ├── ...
│   └── 3-ir-gen
│       ├── report.md                   <- lab3 所需提交的实验报告，请详细说明你们的设计（需要上交）
│       └── contribution.md             <- lab3 所需提交的队员贡献信息（需要上交）
├── src
│   ├── ...
│   └── cminusfc
│       ├── cminusfc.cpp                <- cminusfc 的主程序文件
│       └── cminusf_builder.cpp         <- lab3 需要修改的文件，你们要在该文件中用访问者模式实现自动 IR 生成的算法（需要上交）
└── tests
    ├── ...
    └── 3-ir-gen
        ├── testcases                   <- 助教提供的测试样例
        ├── answers                     <- 助教提供的测试样例
        └── eval.py                     <- 助教提供的测试脚本

```

### 提交要求和评分标准

* 提交要求  
  本实验是组队实验，我们将收取**队长**实验仓库中的内容
  * 实验部分:
    * 需要填补 `./src/cminusfc/cminusf_builder.cpp`
    * 需要在 `./Reports/3-ir-gen/` 目录下撰写实验报告，且由队长说明成员贡献比率
    * 本次实验收取 `./src/cminusfc/cminusf_builder.cpp` 文件和 `./Reports/3-ir-gen` 目录
* 评分标准: 
  * 最终评分按照[组队规则](http://211.86.152.198:8080/staff/2021fall-notice_board/-/issues/46)，实验完成分（总分 60 分）组成如下：
    * cminusf 程序测试 (45分)
    * `report.md`(15分)
  * 禁止执行恶意代码，违者本次实验0分处理
* 迟交规定
  * `Soft Deadline` : 2021/11/21 23:59:59 (北京标准时间，UTC+8)
  * `Hard Deadline` : 2021/11/28 23:59:59 (北京标准时间，UTC+8)
  * 迟交需要邮件通知TA: 
    * 邮箱: lijh2015@mail.ustc.edu.cn
    * 邮件主题: lab3迟交-队长学号
    * 内容: 包括迟交原因、最后版本commitID、迟交时间等
  * 迟交分数
    * x为迟交天数(对于`Soft Deadline`而言)，grade为满分
      ``` bash
      final_grade = grade, x = 0
      final_grade = grade * (0.9)^x, 0 < x <= 7
      final_grade = 0, x > 7 # 这一条严格执行,请对自己负责
      ```
* 关于抄袭和雷同
  经过助教和老师判定属于实验抄袭或雷同情况，所有参与方一律零分，不接受任何解释和反驳。
如有任何问题，欢迎提issue进行批判指正。
