# Lab4 实验文档

- [Lab4 实验文档](#lab4-实验文档)
  - [0. 前言](#0-前言)
    - [主要工作](#主要工作)
      - [阶段一：代码与材料阅读](#阶段一代码与材料阅读)
      - [阶段二：基本优化 pass 开发](#阶段二基本优化-pass-开发)
      - [Lab4 代码与实验报告提交](#lab4代码与实验报告提交)
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

**pass概念**：在lab3中我们已经知道，`module`是 ir 最上层的结构。在本次实验中，`pass`将遍历`module`内的结构，分析出信息(例如对活跃变量的分析 `pass`)，或者是对`module`内的指令和bb做一些变换(例如本次实验中的常量传播与死代码删除和循环不变式外提 `pass`)。

相信大家已经掌握了 lightir 的结构，并且对于 llvm ir 也有了更深的理解。在本次实验中，我们要在理解 ssa （静态单赋值）格式的基础上，实现三个简单的 pass ：常量传播与死代码删除，循环不变式外提，活跃变量分析。

### 主要工作

本次实验分为阶段一与阶段二，分开验收。

#### 阶段一：代码与材料阅读
具体要求与说明参考：[CodeReading.md](./CodeReading.md)

实验目的：

1. 通过阅读优化 pass ，学习优化的基本流程
2. 掌握如何开发基于 lightir 的优化 pass 

实验任务：

* 阅读`Mem2Reg`与`LoopSearch`两个优化 pass 的代码，回答思考题。

请注意查看[lightir](../common/lightir.md)中的User类的`operands_`成员，以及Value类的`use_list_`成员，这两个链表描述了指令间的数据依赖关系。

#### 阶段二：基本优化 pass 开发

1. **常量传播与死代码删除**
   如果一个变量的值可以在编译优化阶段直接计算出，那么就直接将该变量替换为常量（即计算出的结果值）。补充以下几点需要注意的地方：  
    a. 只需要考虑过程内的常量传播，可以不用考虑数组，**全局变量只需要考虑块内的常量传播**，这里举个例子来说明常量传播：
   
    ```cpp
    %a = 1 + 1;
    %b = %a + %c;
    ```
    那么首先我们可以将`%a = 1 + 1`折叠成`%a = 2`，然后我们发现`%b = %a + %c`这条指令用到了`%a`，那么我们就可以将`%b = %a + %c`中的`%a`直接替换成常量2，代码转化为：
    ```cpp
    %a = 2;
    %b = 2 + %c;
    ```
    当然本次实验还需要额外做一步死代码删除：将`%a = 2`这条无用语句删掉，因为`%a`是常量，并且已经传播给了使用它的地方，那么这条赋值语句就可以删掉了（由于我们的 ir 是 ssa 形式，所以不用担心%a被重新赋值）：
    ```cpp
    %b = 2 + %c;
    ```
    b. 整形浮点型都需要考虑。  
    c. 对于`a = 1 / 0`的情形，可以不考虑，即可以做处理也可以不处理。
   
2. **循环不变式外提**
    要能够实现将与循环无关的表达式提取到循环的外面。不用考虑数组与全局变量。举个例子：
    
    ```cpp
    while(i < 10){
      while(j < 10){
        a = i + 1;
        j = j + 1;
      }
      i = i + 1;
    }
    ```
    那么表达式`a=i+1`与内部循环无关，可以提到j循环之外：
    ```cpp
    while(i < 10){
      a = i + 1;
      while(j < 10){
        j = j + 1;
      }
      i = i + 1;
    }
    ```
    下面给出一些循环外提的 tips:  
    a. 思考如何判断语句与循环无关，且外提没有副作用  
    b. 循环的条件块（就是在 LoopSearch 中找到的 Base 块）最多只有两个前驱，思考下，不变式应该外提到哪一个前驱。
    
3. **活跃变量分析**

   能够实现分析 bb 块的入口和出口的活跃变量，参考资料见附件(紫书9.2.4节)。

   在`ActiveVars.hpp`中定义了两个成员`live_in`, `live_out`，你需要：

   1. 将`pair<bb, IN[bb]>`插入`live_in`的 map 结构中（评分将不依据于此变量）。
   2. 将`pair<bb, OUT[bb]>`插入`live_out` 的 map 结构中（评分将依据于此变量）。
   3. 调用`ActiveVars`类中的`print()`方法输出 bb 活跃变量情况到 json 文件。

   **注**：助教会根据你输出的 json 文件中的进行批改。为了保证输出变量名字的一致性，请不要对指令，bb等进行命名操作，`cminusfc_builder`强制要求使用lab3的答案。

   **提示**：材料中没有`phi`节点的设计，数据流方程：

   $`OUT[B] =\cup_{s是B的后继}IN[S]`$   
   
   的定义说明了S入口处活跃的变量在它所有前驱的出口处都是活跃的。由于`phi`指令的特殊性，例如`%0 = phi [%op1, %bb1], [%op2, %bb2]`如果使用如上数据流方程，则默认此`phi`指令同时产生了`op1`与`op2`的活跃性。事实上，只有控制流从`%bb1`传过来phi才产生`%op1`的活跃性，从`%bb2`传过来phi才产生`%op2`的活跃性。因此对此数据流方程需要做一些调整：  
   
   $`OUT[B] =\cup_{s是B的后继}IN[S]\cup_{s是B的后继} phi\_uses[S,B]`$。  
   
   其中`IN[S]`是S中剔除`phi`指令后分析出的入口变量结果。`phi_uses[S,B]`表示S中的`phi`指令参数中`label`为B的对应变量。举例如下：

   ```c
   label4:                                                ; preds = %label_entry, %label59
     %op62 = phi i32 [ %op65, %label59 ], [ undef, %label_entry ]
     %op63 = phi i32 [ 0, %label_entry ], [ %op61, %label59 ]
     %op64 = phi i32 [ %op66, %label59 ], [ undef, %label_entry ]
     %op6 = icmp slt i32 %op63, 10000000
     %op7 = zext i1 %op6 to i32
     %op8 = icmp ne i32 %op7, 0
     br i1 %op8, label %label9, label %label10
   ```

   在这个基本块中 `phi_uses[label4,label59]`={`%op65`，`%op66`，`%op61`}。

   

#### Lab4代码与实验报告提交
1. 基本优化pass的代码都写在`src/optimization/`目录下面，头文件放入`include/optimization/`当中，最后只会在这两个目录下验收代码文件。
2. 需要在 `Reports/4-ir-opt/` 目录下撰写实验报告，且由队长说明成员贡献比率。其中，在 `report-phase1.md` 中完成阶段一的代码阅读部分的报告，在 `report-phase2.md` 中解释阶段二中的基本优化pass的设计，遇到的困难和解决方案，由**队长**在 `contribution.md` 中解释每位队员的贡献，并说明贡献比例

注意：组队实验意味着合作，但是小组间的交流是受限的，且严格**禁止**代码的共享。除此之外，如果小组和其它组进行了交流，必须在 `report-phase1.md` 和`report-phase2.md`  中记录下来交流的小组和你们之间交流内容。



## 1. 实验框架

本次实验使用了由 C++ 编写的 lightir 来在 ir 层面完成优化化简，在`include/optimization/passManager.hpp`中，定义了一个用于管理 pass 的类`passManager`。它的作用是注册与运行 pass 。它提供了以下接口：

```cpp
passManager pm(module.get())
pm.add_pass<Mem2Reg>(emit)	//注册 pass， emit 为true时打印优化后的 ir
pm.run()	//按照注册的顺序运行 pass 的 run() 函数
```
基本 pass 开发：  
- 每一个 pass 有一个 cpp 文件和对应的 hpp 文件，可以在 hpp 里定义辅助类或者成员变量使用，在 cpp 里的`run()`函数实现你的 pass 。



## 2. 运行与调试

### 运行 cminusfc

```sh
mkdir build && cd build
cmake ..
make -j
make install
```
编译后会产生 `cminusfc` 程序，它能将 cminus 文件输出为 llvm ir ，也可以利用 clang 将 ir 编译成二进制。程序逻辑写在`cminusfc.cpp`中。

为了便于大家进行实验，助教对之前的`cminusfc`增加了选项，用来选择是否开启某种优化，通过`[-mem2reg] [-const-propagation] [-active-vars] [-loop-invariant]`开关来控制优化 pass 的使用，当需要对 `.cminus` 文件测试时，可以这样使用：
```bash
./cminusfc [-mem2reg] [-const-propagation] [-active-vars] [-loop-invariant] <input-file>
```
另外，若想要另外单独去进行某个优化 pass 的调试，可以利用助教给出的 passManager 来进行 pass 的注册和运行。



### 自动测试

助教贴心地为大家准备了自动测试脚本，它在 `tests/4-ir-opt` 目录下，使用方法如下：
* 有三个可用的选项：`--ConstPropagation`/`-C`，`--LoopInvHoist`/`-L`，`--ActiveVars/-A`分别表示用来评测常量传播与死代码删除 pass 以及循环不变式外提 pass ，以及活跃变量分析 pass。

* 脚本中会使用`taskset`将程序与 CPU 核心进行绑定，以此来提高时间测试的稳定性；当然如果虚拟机中没有该命令则通过下面的命令来安装：
  ```bash
  sudo apt install schedtool
  ```
  
* 评测脚本会对样例进行编译和执行，然后对生成的可执行文件首先检查结果的正确性，每个样例的正确结果会放在`.out`文件中，结果正确的情况下才会去进一步评测运行时间。另外，在每类样例目录下中的`baseline`目录中还提供了相应 testcase 的`.ll`文件来作为 baseline ，基本 pass 的优化效果得分也是要根据`baseline`的时间来进行计算。

* 如果显示执行时间的表格中出现了`None`则表示该样例有错误。

* 每个样例会运行三次取平均时间（时间单位是 s ）并且保留两位小数输出，当然每个样例的运行次数也可以自行更改脚本中`repeated_time`变量。

* 活跃变量 pass 测试将与答案 json 文件脚本对比，得分计算规则见下面评分标准
```sh
# 在 tests/4-ir-opt 目录下运行：
./lab4_tests.py -L
```
如果完全正确，它会输出：
```
========== LoopInvHoist ==========
Compiling  
100%|███████████████| 8/8 [00:00<00:00, 12.16it/s]
Evalution 
100%|███████████████| 8/8 [00:49<00:00,  6.14s/it]
Compiling  -loop-inv-hoist
100%|███████████████| 8/8 [00:00<00:00, 11.85it/s]
Evalution 
100%|███████████████| 8/8 [00:10<00:00,  1.25s/it]
Compiling baseline files
100%|███████████████| 8/8 [00:00<00:00, 13.63it/s]
Evalution 
100%|███████████████| 8/8 [00:07<00:00,  1.09it/s]
testcase         before optimization     after optimization      baseline
testcase-1              0.63                    0.36              0.36
testcase-2              0.46                    0.38              0.37
testcase-3              0.62                    0.36              0.36
testcase-4              0.40                    0.39              0.39
testcase-5              4.96                    0.38              0.38
testcase-6              1.03                    0.08              0.08
testcase-7              2.11                    0.24              0.24
testcase-8              1.98                    0.25              0.25
```
如果要增加样例，直接在样例目录中添加文件即可，命名参考目录下的其他文件。


### logging

[logging](../common/logging.md) 是帮助大家打印调试信息的工具，如有需求可以阅读文档后进行使用

### 建议

1. 比较你们编写的编译器产生的 ir 和 clang 产生的 ir 来找出可能的问题或发现新的思路
2. 使用 logging 工具来打印调试信息
2. 使用 gdb 等软件进行单步调试来检查错误的原因
3. 合理分工

## 3. 提交要求

### 目录结构

```
.
├── CMakeLists.txt
├── Documentations
│   ├── ...
│   ├── common
│   |   ├── lightir.md                  <- lightir 相关文档
│   |   ├── logging.md                  <- logging 工具相关文档
│   |   └── cminusf.md                  <- cminus-f 的语法和语义文档
│   └── 4-ir-opt
│       ├── 活跃变量.pdf                 <- 活跃变量部分相关文档
│       ├── 静态单赋值格式构造.pdf         <- mem2reg文献阅读相关文档
│       ├── CodeReading.md              <- 代码阅读文档
│       └── README.md                   <- Lab4 实验文档说明（你在这里）
├── include                             <- 实验所需的头文件
│   ├── ...
│   ├── optimization/*
│   ├── cminusf_builder.hpp
|   └── ast.hpp
├── Reports
│   ├── ...
│   └── 4-ir-opt
│       ├── report-phase1.md            <- Lab4 所需提交的实验阅读部分报告，请详细说明你们的设计（需要上交）
│       ├── report-phase2.md            <- Lab4 所需提交的实验基本pass实验报告，请详细说明你们的设计（需要上交）
│       └── contribution.md             <- Lab4 所需提交的队员贡献信息（需要上交）
├── src
│   ├── ...
│   └── optimization
│       ├── LoopSearch.cpp                <- 循环查找
│       ├── Mem2Reg.cpp                   <- Mem2Reg
│       ├── Dominators.cpp                <- 支配树
│       ├── ActiveVars.cpp                <- 活跃变量
│       ├── ConstPropagation.cpp          <- 常量传播与死代码删除
│       └── LoopInvHoist.cpp              <- 循环不变式外提        
│       
└── tests
    ├── ...
    └── 4-ir-opt
        ├── testcases                   <- 助教提供的测试样例
        └── lab4_tests.py                <- 助教提供的测试脚本

```
### 提交要求和评分标准

- 提交时间

  本次实验分阶段验收：

  **阶段一**：验收代码阅读报告及相关思考题`report-phase1.md`

  **阶段二**：验收Lab4要求提交的代码及`report-phase2.md`

* 提交要求  
  本实验是组队实验，我们将收取**队长**实验仓库中的内容
  
  * 需要填补 
  
    `./include/optimization/ActiveVars.hpp`，`./include/optimization/ConstPropagation.hpp`，`./include/optimization/LoopInvHoist.hpp` `./src/optimization/ActiveVars.cpp`，`./src/optimization/ConstPropagation.cpp.cpp`，`./src/optimization/LoopInvHoist.cpp`
  
  * 需要在 `./Reports/4-ir-opt/` 目录下撰写实验报告，且由队长说明成员贡献比率
  
  * 本次实验收取 
  
    `./include/optimization/ActiveVars.hpp`，`./include/optimization/ConstPropagation.hpp`，`./include/optimization/LoopInvHoist.hpp` 
  
    `./src/optimization/ActiveVars.cpp`，`./src/optimization/ConstPropagation.cpp`，`./src/optimization/LoopInvHoist.cpp` 文件和 `./Reports/4-ir-opt` 目录下报告，其中`report-phase1.md` 会在阶段一的ddl的时候进行验收；`report-phase2.md`以及实现代码会在阶段二的ddl的时候进行验收。
  
* 评分标准: 最终评分按照[组队规则](http://211.86.152.198/staff/2021fall-notice_board/-/issues/46)，实验完成分（总分 60 分）组成如下：
  * 阶段一 代码阅读
    
    * report-phase1.md (5 分)
  * 阶段二 优化pass开发
    * **基本pass (55 分)**
      
      * report-phase2.md (10 分)
      
      * 常量传播与死代码删除 (15 分)
        ```
        对于每一个testcase: 
        (before_optimization-after_optimization)/(before_optimization-baseline) > 0.8 得满分
        (before_optimization-after_optimization)/(before_optimization-baseline) > 0.5 得85%分数
        (before_optimization-after_optimization)/(before_optimization-baseline) > 0.2 得60%分数
        ```
        **注**：`before_optimization`以Lab3答案为基准，Lab4代码不收取`cminusf_builder.cpp`
        
        若编译出错或者运行出错将不得分，此外评测时所用的`testcase`与发布的不完全一致，最终的评分会映射到15分的总分区间。
        
      * 循环不变式外提 (15 分)
        评分参考常量传播与死代码删除。
        
      * 活跃变量 (15 分)
      
        活跃变量的一个bb的出口处活跃变量的非空集合算一个分析结果，每个分析结果同分，对于每个分析结果`result`评分采取以下公式，（正确分析结果列表是`answer`）
        ```math
        score = \frac{(answer\cap result).size()-(result-answer\cap result).size()}{answer.size()}
        ```
        例如：
      
        ```json
        "live_out":{
            //...
            "label8": ["%arg0","%arg1","%op5","%op6","%op8",]
        	//...
        }
        // 算一个分析结果，如果正确答案是如下结果，
        "live_out":{
            //...
            "label8": ["%arg0","%arg1","%op5","%op6","%op7",]
        	//...
        }
        // 则相比较于答案，缺少了op7，多分析了op8则此条分析结果得分为(4-1)/5=0.6
        ```
      
    
  * 禁止执行恶意代码，违者本次实验0分处理
  
* 迟交规定
  * `Soft Deadline` :
  
    **阶段一**：2021/11/29 23:59:59 (北京标准时间，UTC+8)  
  
    **阶段二**：2022/12/13 23:59:59 (北京标准时间，UTC+8) 
  
  * `Hard Deadline`：
  
    **阶段一**：2021/12/06 23:59:59 (北京标准时间，UTC+8)  
  
    **阶段二**：2022/12/20 23:59:59 (北京标准时间，UTC+8) 
  
  * 迟交需要邮件通知 TA : 
    * 邮箱: 
    chen16614@mail.ustc.edu.cn
    * 邮件主题: Lab4迟交-队长学号
    * 内容: 包括迟交原因、最后版本commitID、迟交时间等
    
  * 迟交分数
    * x为迟交天数(对于`Soft Deadline`而言)，grade为满分
      ``` bash
      final_grade = grade, x = 0
      final_grade = grade * (0.9)^x, 0 < x <= 7
      final_grade = 0, x > 7 # 这一条严格执行,请对自己负责
      ```
  
* 关于抄袭和雷同
  经过助教和老师判定属于实验抄袭或雷同情况，所有参与方一律零分，不接受任何解释和反驳（严禁对开源代码或者其他同学代码的直接搬运）。
  如有任何问题，欢迎提issue进行批判指正。
