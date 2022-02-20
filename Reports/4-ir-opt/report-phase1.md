# Lab4 实验报告-阶段一

**小组成员**

PB19050946 郑雨霏 

PB19050991 陶思成  

PB19111659 胡冰

## 实验要求

完成**常量传播与死代码删除**、**循环不变式外提**、**活跃变量分析**对应的 `.hpp` 和 `.cpp`，实现对静态单赋值格式的中间代码的优化。

## 思考题
### LoopSearch
1. `LoopSearch`中直接用于描述一个循环的数据结构是什么？需要给出其具体类型。

   >描述一个循环的数据结构是一个结构体指针无序集合容器，其具体表示如下：
   >
   >  ```
   >  using CFGNodePtrSet = std::unordered_set<CFGNode*>;
   >  //其中CFGNode为结构体struct CFGNode;
   >  ```
   >

2. 循环入口是重要的信息，请指出`LoopSearch`中如何获取一个循环的入口？需要指出具体代码，并解释思路。

   >具体代码如下所示
   >```C++
   >CFGNodePtr LoopSearch::find_loop_base(//找到循环的中的入口
   >   CFGNodePtrSet *set,//指向当前循环（包含所有基本块）的指针
   >   CFGNodePtrSet &reserved)//删去的循环入口基本块的保留集合
   >{
   >
   >   CFGNodePtr base = nullptr;//先初始将入口指针设置为nullptr
   >   for (auto n : *set)//依次遍历当前循环中的基本块
   >   {
   >      for (auto prev : n->prevs)//依次遍历当前访问的基本块的前驱
   >      {
   >            if (set->find(prev) == set->end())//判断是否存在一个前驱不在该集合内部，若存在满足情况的前驱，则当前基本块就是当前循环的入口
   >            {
   >               base = n;
   >            }
   >      }
   >   }
   >   if (base != nullptr)//若找到入口，则返回
   >      return base;
   >   for (auto res : reserved)//否则遍历那些已经删除的基本块
   >   {
   >      for (auto succ : res->succs)//依次遍历当前访问的基本块的后继
   >      {
   >            if (set->find(succ) != set->end())//判断是否存在一个后继在该集合内部，若存在，则这个后继就是当前循环的入口
   >            {
   >               base = succ;
   >            }
   >      }
   >   }
   >//返回找到的循环入口
   >   return base;
   >}
   >```

3. 仅仅找出强连通分量并不能表达嵌套循环的结构。为了处理嵌套循环，`LoopSearch`在Tarjan algorithm的基础之上做了什么特殊处理？

   >将当前找到的循环入口在图中删去，并删去其前驱结点中指向该结点的后继指针以及其后继中指向该结点的前驱指针，然后再重新用`Tarjan algorithm`对于当前的图进行强连通分量查找，以找到内部循环

4. 某个基本块可以属于多层循环中，`LoopSearch`找出其所属的最内层循环的思路是什么？这里需要用到什么数据？这些数据在何时被维护？需要指出数据的引用与维护的代码，并简要分析。

   > 1. 与上题类似，找到该层循环的入口后，通过变量 `bb2base` 将该强连通分量中每个节点（基本块）与该循环入口对应，然后删除当前的循环入口并删去前驱节点和后继节点中保存的该循环入口。
   >
   > 2. 重新构造 strongly_connected_components，可以得到不含最外层循环的强连通分量，重复上一步骤，则内层循环的基本块对应的循环入口会得到更新，直到达到最内层循环。
   >
   > - 用到的数据
   >
   >   ```c++
   >   // { bb :  entry bb of loop} 默认最低层次的loop
   >   std::unordered_map<BasicBlock *, BasicBlock *> bb2base;
   >   
   >   ```
   >
   > - 在 `void LoopSearch::run()` 第5步中引用和维护，分析如下
   >
   >   ```cpp
   >   // step 5: map each node to loop base
   >   for (auto bb : *bb_set)
   >   {
   >       if (bb2base.find(bb) == bb2base.end())
   >           // 如果是第一次碰到该基本块，将该基本块和该层循环入口的基本块压入 bb2base 中
   >           bb2base.insert({bb, base->bb});
   >       else
   >           // 再次碰到该基本块（在内层循环中），更新该基本块对应的循环入口
   >           bb2base[bb] = base->bb;
   >   }
   >   ```
   >
### Mem2reg
1. 请**简述**概念：支配性、严格支配性、直接支配性、支配边界。

   >支配性：对于节点 n 和节点 m，如果到达 m 的每条代码路径都必然经过 n，则称 n 支配 m，记为 $n \in Dom(m)$。
   >
   >严格支配性：节点n 支配节点 m 且 $n \neq m$，则称 n 严格支配 m，即 $n \in Dom(m)-m$。
   >
   >直接支配性：节点 n 是 $Dom(m)-m$ 中与 m  最接近的节点。
   >
   >支配边界：节点 n 支配节点 m 的一个前驱，但不严格支配节点 m，则称 m 为 n 的支配边界，记为 $DF(n)$

2. `phi`节点是SSA的关键特征，请**简述**`phi`节点的概念，以及引入`phi`节点的理由。

   >phi 节点概念：在当前块的前驱节点中某个变量的所有值中选择一个 ，即将同一个变量的所有赋值汇聚到一个变量中。
   >
   >引入 phi 节点的理由：静态单赋值每个变量只赋值一次，则同一个变量多次赋值时会由不同的变量保存，在后面引用该变量时需要考虑从引用哪个变量中保存的值，phi 节点即用来处理该问题。

3. 下面给出的`cminus`代码显然不是SSA的，后面是使用lab3的功能将其生成的LLVM IR（**未加任何Pass**），说明对一个变量的多次赋值变成了什么形式？

   >SSA形式要求其IR中的每个变量只能被赋值一次，这就意味着在cminus中对同一个变量的多次赋值，在SSA形式下会带来多个变量的生成与赋值。在cminus中这个变量每次作为左值被调用，对应在SSA中就生成一个新的变量

4. 对下面给出的`cminus`程序，使用lab3的功能，分别关闭/开启`Mem2Reg`生成LLVM IR。对比生成的两段LLVM IR，开启`Mem2Reg`后，每条`load`, `store`指令发生了变化吗？变化或者没变化的原因是什么？请分类解释。

   >我们将程序中的`load,store`指令分为3类：1）没有变化 2) 因为`generate_phi`导致变化 3）因为`re_name`导致变化
   >
   >- 1)没有变化的是有
   >
   >  ```
   >  store i32 1, i32* @globVar
   >  store i32 999, i32* %op5
   >  ```
   >
   >  第一个`store`指令没有发生变化是因为右值是全局变量
   >
   >  第二个`store`指令没有发生变化是因为右值是gep生成的变量
   >
   >- 2)因为`generate_phi`导致变化
   >
   >  ```
   >  store i32 0, i32* %op1
   >  %op8 = load i32, i32* %op1
   >  ```
   >
   >  第一个`store`指令变化是因为由于`phi`指令的使用，此处无需在对%op1额外进行赋值
   >
   >  第二个`load`指令变化是因为引入`phi`指令后，值直接由`phi`赋予
   >
   >- 3）因为`re_name`导致变化
   >
   >  ```
   >  store i32 %arg0, i32* %op1
   >  store i32 2333, i32* %op1
   >  ```
   >
   >  这两处的`store`变化都是由于`re_name`进行优化后被删去了

5. 指出放置phi节点的代码，并解释是如何使用支配树的信息的。需要给出代码中的成员变量或成员函数名称。

   > 放置phi节点的代码如下所示：
   >
   > ```c++
   > // step 2: insert phi instr
   >     std::map<std::pair<BasicBlock *,Value *>, bool> bb_has_var_phi; // bb has phi for var
   >     for (auto var : global_live_var_name )
   >     {
   >         std::vector<BasicBlock *> work_list;
   >         work_list.assign(live_var_2blocks[var].begin(), live_var_2blocks[var].end());
   >         for (int i =0 ; i < work_list.size() ; i++ )
   >         {   
   >             auto bb = work_list[i];
   >             for ( auto bb_dominance_frontier_bb : dominators_->get_dominance_frontier(bb))
   >             {
   >                 if ( bb_has_var_phi.find({bb_dominance_frontier_bb, var}) == bb_has_var_phi.end() )
   >                 { 
   >                     // generate phi for bb_dominance_frontier_bb & add bb_dominance_frontier_bb to work list
   >                     auto phi = PhiInst::create_phi(var->get_type()->get_pointer_element_type(), bb_dominance_frontier_bb);
   >                     phi->set_lval(var);
   >                     bb_dominance_frontier_bb->add_instr_begin( phi );
   >                     work_list.push_back( bb_dominance_frontier_bb );
   >                     bb_has_var_phi[{bb_dominance_frontier_bb, var}] = true;
   >                 }
   >             }
   >         }
   >     }
   > ```
   >
   > - `dominators`:在程序最开始就初始化，并已经运行了`run`函数，构建出了一颗完整的支配树，现在只需要调用获取信息即可。
   >
   > - `get_dominance_frontier`: 从支配树中取出当前`BasicBlock`对应流图中结点的支配边界，用于插入`phi`函数
   >
   >   基本流程： 从上文获取到的`global_live_var_name`列表中取出变量`var`, 在`live_var_2blocks`列表中获取`var`所在的所有`BasicBlock`并保存在`work_list`。对`BasicBlock`进行遍历，取`BasicBlock`在支配树中的支配边界，为支配边界上的`BaicBlock`添加`phi`指令，同时将这个`BasicBlock`添加到`work_list`的最后，并将已经添加`phi`的`BasicBlock`标记为`True`。照这样一直处理到`work_list`被全部遍历过。

### 代码阅读总结

对循环的处理、静态单赋值的处理有了深入的认识，理解了变量、函数的作用，对要求实现的三个 pass 有了初步构思。

明白了优化 pass 的实现流程，以及在 `hpp` 和 `.cpp` 中编写代码的规范。

### 实验反馈 （可选 不会评分）

实验设计很用心

### 组间交流 （可选）

本次实验和哪些组（记录组长学号）交流了哪一部分信息
