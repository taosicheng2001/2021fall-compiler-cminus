# 材料与代码阅读

## 前言
阅读代码前请仔细阅读下面关于`LoopSearch`和`Mem2Reg`部分的说明，在经过队内讨论后在组长仓库中的`Report/report-phase1.md`中写下**思考题**的答案。

`Control Flow Graph`(以下简称为`CFG`)是一种用有向图来描述程序执行过程的方式，它是对程序块之间的控制流建立的联系。其中，图中的节点表示基本块(`BasicBlock`)，基本块中，除了最后一条指令，其余的指令都是顺序执行不会产生分支跳转；图中的边则表示可能的控制转移。


通过对`CFG`的分析，可以找到代码的许多重要结构信息，比如循环。所以`CFG`对于代码优化有着重要的意义，而本次实验的优化Pass，都是建立在对`CFG`的分析与修改上的。在之前的实验中，同学们通过调用LightIR的接口(参考[LightIR介绍](../common/LightIR.md))完成了中间代码生成，同时也通过核心类建立和维护着中间代码的`CFG`，核心类中，`BasicBlock`就可以看作是`CFG`的基本块，类中的成员`pre_bbs_`与`succ_bbs_`就可以记下每个块的前驱后继，通过递归地访问每个块的上述两个成员，就可以遍历整个`CFG`。在本次代码阅读实验部分，同学们请仔细阅读样例代码，掌握如何去调用`LightIR`的接口来开发优化Pass。



## LoopSearch
#### 简介
循环查找是属于一种**分析类Pass**，该类Pass是用来为之后的优化Pass获取`CFG`的必要信息，而不会对`CFG`本身造成改变；循环查找的主要目的就是为了找到程序当中的所有循环，以及这些循环的入口块（即循环的条件块）等信息。**该分析Pass也是后续的循环不变式外提优化的基础。**
#### 代码阅读说明
1. 代码相关的两个文件分别是`src/optimization/LoopSearch.cpp`以及`include/optimization/LoopSearch.hpp`
2. 循环查找会用到图强连通分量搜索算法--Tarjan algorithm，如果代码阅读时遇到问题可以自行查找相关资料辅助阅读。
3. 为了方便阅读以及后续实验debug方便，该模块还设计了一个将找到的所有循环可视化的模块。使用该模块有以下几点需要注意：
    * 确保环境中安装了Graphviz，如果没有可以使用命令安装：
    ```bash
    sudo apt-get install graphviz
    ```
    * 每个BasicBlock都需要命名，且命名要符合规范。
    * 该接口是通过`LoopSearch`的构造函数中的dump参数来控制是否打印找到的循环，dump为true时会打印，否则将不打印，发布的代码中是**默认不打印的**：
    ```cpp
    explicit LoopSearch(Module* m, bool dump=false) : Pass(m), dump(dump){}
    ```
    * 该功能会在当前目录下输出png图片文件，文件有两种命名：
        a. `function_name`.png：表示某个函数内所有的basic block连接。
        b. `function_name`_`index`.png：表示在某函数中第`index`个循环。
4. 重要的成员变量和函数说明（后续实验会用到，阅读代码需要着重理解）：
    * `loop_set`：算法找到的所有循环的集合
    * `func2loop`：记录函数与循环的对应关系
    * `base2loop`：记录base块与循环的对应关系
    * `loop2base`：记录base块与循环的对应关系
    * `bb2base`：记录循环中的一个块到这个块所在循环（最内层循环）的入口的映射关系。
    * `get_loop_base`：得到循环的入口块
    * `get_inner_loop`：得到bb所在最低层次的循环
    * `get_parent_loop`：得到输入loop的外一层的循环，如果没有则返回空
    * `get_loops_in_func`：得到某个函数内的所有循环


#### 思考题
1. `LoopSearch`中直接用于描述一个循环的数据结构是什么？需要给出其具体类型。

2. 循环入口是重要的信息，请指出`LoopSearch`中如何获取一个循环的入口？需要指出具体代码，并解释思路。

3. 仅仅找出强连通分量并不能表达嵌套循环的结构。为了处理嵌套循环，`LoopSearch`在Tarjan algorithm的基础之上做了什么特殊处理？

4. 某个基本块可以属于多层循环中，`LoopSearch`找出其所属的最内层循环的思路是什么？这里需要用到什么数据？这些数据在何时被维护？需要指出数据的引用与维护的代码，并简要分析。

   


## Mem2Reg

#### 简介

`Mem2Reg Pass`构造了LLVM IR 的SSA格式(静态单赋值格式)。关于SSA是什么，为什么要构建SSA格式，以及如何构建附件材料中均有提及，也可以通过自行搜索材料了解。

#### 代码说明

1. 代码相关的四个文件分别是`src/optimization/Dominators.cpp` `src/optimization/Mem2Reg.cpp`以及`include/optimization/Dominators.hpp` `include/optimization/Mem2Reg.hpp`
2. 其中`Dominators`是生成支配树信息，被`Mem2Reg`调用。
3. `Mem2Reg Pass`的执行流程与算法**伪代码详见附件**：(提示：注意了解其中的phi函数节点)

#### 思考题：（描述清楚即可，不需要写很多字数）

1. 请**简述**概念：支配性、严格支配性、直接支配性、支配边界。

2. `phi`节点是SSA的关键特征，请**简述**`phi`节点的概念，以及引入`phi`节点的理由。

3. 下面给出的`cminus`代码显然不是SSA的，后面是使用lab3的功能将其生成的LLVM IR（**未加任何Pass**），说明对一个变量的多次赋值变成了什么形式？

   `cminus`代码：

   ```c
   int main(void){
       int a;
       a = 0;
       a = 1 + 2;
       a = a * 4;
       return 0;
   }
   ```

   生成的LLVM IR：

   ```c
   ; ModuleID = 'cminus'
   source_filename = "non_ssa.cminus"
   
   declare i32 @input()
   
   declare void @output(i32)
   
   declare void @outputFloat(float)
   
   declare void @neg_idx_except()
   
   define i32 @main() {
   label_entry:
     %op0 = alloca i32
     store i32 0, i32* %op0
     %op1 = add i32 1, 2
     store i32 %op1, i32* %op0
     %op2 = load i32, i32* %op0
     %op3 = mul i32 %op2, 4
     store i32 %op3, i32* %op0
     ret i32 0
   }
   ```

   

4. 对下面给出的`cminus`程序，使用lab3的功能，分别关闭/开启`Mem2Reg`生成LLVM IR。对比生成的两段LLVM IR，开启`Mem2Reg`后，每条`load`, `store`指令发生了变化吗？变化或者没变化的原因是什么？请分类解释。

   `cminus`代码：

   ```c
   int globVar;
   int func(int x){
       if(x > 0){
           x = 0;
       }
       return x;
   }
   int main(void){
       int arr[10];
       int b;
       globVar = 1;
       arr[5] = 999;
       b = 2333;
       func(b);
       func(globVar);
       return 0;
   }
   ```

   关闭`Mem2Reg`生成的LLVM IR：

   ```c
   ; ModuleID = 'cminus'
   source_filename = "mem2reg_example.cminus"
   
   @globVar = global i32 zeroinitializer
   declare i32 @input()
   
   declare void @output(i32)
   
   declare void @outputFloat(float)
   
   declare void @neg_idx_except()
   
   define i32 @func(i32 %arg0) {
   label_entry:
     %op1 = alloca i32
     store i32 %arg0, i32* %op1
     %op2 = load i32, i32* %op1
     %op3 = icmp sgt i32 %op2, 0
     %op4 = zext i1 %op3 to i32
     %op5 = icmp ne i32 %op4, 0
     br i1 %op5, label %label6, label %label7
   label6:                                                ; preds = %label_entry
     store i32 0, i32* %op1
     br label %label7
   label7:                                                ; preds = %label_entry, %label6
     %op8 = load i32, i32* %op1
     ret i32 %op8
   }
   define i32 @main() {
   label_entry:
     %op0 = alloca [10 x i32]
     %op1 = alloca i32
     store i32 1, i32* @globVar
     %op2 = icmp slt i32 5, 0
     br i1 %op2, label %label3, label %label4
   label3:                                                ; preds = %label_entry
     call void @neg_idx_except()
     ret i32 0
   label4:                                                ; preds = %label_entry
     %op5 = getelementptr [10 x i32], [10 x i32]* %op0, i32 0, i32 5
     store i32 999, i32* %op5
     store i32 2333, i32* %op1
     %op6 = load i32, i32* %op1
     %op7 = call i32 @func(i32 %op6)
     %op8 = load i32, i32* @globVar
     %op9 = call i32 @func(i32 %op8)
     ret i32 0
   }
   ```

   开启了`Mem2Reg`生成的LLVM IR：

   ```c
   ; ModuleID = 'cminus'
   source_filename = "mem2reg_example.cminus"
   
   @globVar = global i32 zeroinitializer
   declare i32 @input()
   
   declare void @output(i32)
   
   declare void @outputFloat(float)
   
   declare void @neg_idx_except()
   
   define i32 @func(i32 %arg0) {
   label_entry:
     %op3 = icmp sgt i32 %arg0, 0
     %op4 = zext i1 %op3 to i32
     %op5 = icmp ne i32 %op4, 0
     br i1 %op5, label %label6, label %label7
   label6:                                                ; preds = %label_entry
     br label %label7
   label7:                                                ; preds = %label_entry, %label6
     %op9 = phi i32 [ %arg0, %label_entry ], [ 0, %label6 ]
     ret i32 %op9
   }
   define i32 @main() {
   label_entry:
     %op0 = alloca [10 x i32]
     store i32 1, i32* @globVar
     %op2 = icmp slt i32 5, 0
     br i1 %op2, label %label3, label %label4
   label3:                                                ; preds = %label_entry
     call void @neg_idx_except()
     ret i32 0
   label4:                                                ; preds = %label_entry
     %op5 = getelementptr [10 x i32], [10 x i32]* %op0, i32 0, i32 5
     store i32 999, i32* %op5
     %op7 = call i32 @func(i32 2333)
     %op8 = load i32, i32* @globVar
     %op9 = call i32 @func(i32 %op8)
     ret i32 0
   }
   ```

   

5. 指出放置phi节点的代码，并解释是如何使用支配树的信息的。需要给出代码中的成员变量或成员函数名称。

