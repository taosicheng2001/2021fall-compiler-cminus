# Lab4 实验报告

**小组成员**

PB19050946 郑雨霏 

PB19050991 陶思成  

PB19111659 胡冰

## 实验要求

请按照自己的理解，写明本次实验需要干什么

​	本次实验的目的是对上个Lab3中的编译器生成的llvm中间代码做进一步优化，以达到减少代码运行时间的效果。所以本次实验的主要要求有三个：

1. 保证代码优化后代码的正确性。
2. 通过一系列手段，减少代码运行时执行的代码条数。
3. 计算活跃变量，这个在其他优化的`PASS`中，比如删除无用变量中可以用到。

我们本次试验就是基于以上三点展开，在保证中间代码在每个`PASS`之间的变换为等价变换的前提下，尽可能地减少运行时代码条数。本实验最终完成的代码优化包括：

1. 常量传播

   - 四则运算表达式的常量折叠
   - 比较表达式的常量折叠
   - 零拓展指令常量折叠
   - 类型转化指令常量折叠
   - `load`和`store`联合使用时的常量传播

   - 全局变量的常量传播
2. 死代码删除

   - 常真常假跳转的删除
   - 单来源`Phi`指令的删除
3. 循环不变量外提

   - 四则运算结果的循环不变量外提
   - 赋值`load`时的循环不变量外提
   - 分配空间时的循环不变量外提
   - 类型转化时的循环不变量外提
   - 比较表达式时的循环不变量外提
   - 零拓展指令时的循环不变量外提
   - 取数组值时的循环不变量外提
4. 活跃变量分析
   - 计算每个基本块的 IN  和 OUT


## 实验难点

实验中遇到哪些挑战

- **常量传播**

  - 全局变量的常量传播实现
  - `load`和`store`联合使用时的常量传播实现

- **循环不变量外提**

  - 实现所有可外提的循环不变量的外提

    先遍历所有循环，存储所有循环的最内部循环，由内循环向外循环逐层实现循环不变量外提，可以使得所有可外提的循环均被外提

    ```C++
        //遍历所有循环，找到每个循环的最内存循环,将最内层的循环入口放入inner_loops中，可以避免重复查找
    		InnerloopSet inner_loops;
        auto func_list = m_->get_functions();
        for(auto func: func_list){
            auto loops = loop_searcher.get_loops_in_func(func);
            for(auto loop: loops){
                auto inner_loop = loop_searcher.get_inner_loop(loop_searcher.get_loop_base(loop));
                inner_loops.insert(inner_loop);
            }
        }
    		for(auto inner_loop: inner_loops){
            auto in2out_loop = inner_loop;
            //由内而外查找每个循环中的循环不变量
                while(in2out_loop !=nullptr){
                  ···
                  ···
                }
                in2out_loop = loop_searcher.get_parent_loop(in2out_loop);
            }
        }
    ```
    
  - **存储所有需要被外提的循环不变量**
  
    初始时所有结构体均使用`std::unordered_set`，所以当存在两个循环不变量相同时，后一个不会有任何操作，所以会使运行测试脚本时发生答案错误的现象，后改用`std::vector`，将所有循环不变量通过push_back存入其中，则不会发生之前的错误
    ```C++
    //using MoveSet = std::unordered_set<Instruction*>   =>
    using MoveSet = std::vector<Instruction*>;
    ```
    
  
- **活跃变量分析**

  - 分析指令时，`call` 指令和 `phi` 指令稍有难度

    **`call`** 指令格式： `<result> = call <return ty> <func name>(<function args>) `

    在使用 `get_operand()` 遍历等号右边参数时，要跳过 `<func name>`。

    返回值可能为 `void` 类型，故需判断左值是否存在，再进行 `def` 计算。

    ```cpp
    else if(instr->is_call()){ // 左值不定，右值不定
        auto _num = instr->get_num_operand();
        for(int i = 1; i < _num; i++){
            // 参数0保存调用的函数名字，需要跳过
            auto args = instr->get_operand(i);
    
            if (!IS_CONSTANT_INT(args) && !IS_CONSTANT_FP(args)
                && tmp_def.find(args)==tmp_def.end()){
                // 右值 args 不是常量，且引用前无定值
                tmp_use.insert(args);
            }                        
        }
    
        // 左值 0 个或 1 个
        if (!instr->is_void() // 有返回值（一个左值）
            && tmp_use.find(instr) == tmp_use.end()){
            // 左值 instr 定值前无引用
            tmp_def.insert(instr);
        }
    }
    ```

    **`phi`** 指令格式： `<result> = call <type> (<op> <bb>) `

    `op `为变量或常量名，`bb` 是 `op` 来自的基本块。在处理 `phi` 指令包含的活跃变量时，要同时保存该活跃变量由哪个基本块传过来，存放在 `phi_use` 中（声明如下）。`phi_use` 保存基本块产生的所有 `phi`活跃变量（这些变量可能来自不同块的不同 `phi` 指令）。

    ```cpp
    std::map<BasicBlock *, std::set<Value *>> phi_use;
    ```

    ```cpp
    else if(instr->is_phi()){ // 一个左值，右值不定保存到phi_use中
        auto _num = instr->get_num_operand();
        for(int i = 0; i < _num; i+=2){
            // [%avar, %lable]
    
            auto val = instr->get_operand(i);
            auto pre_bb = dynamic_cast<BasicBlock *>(instr->get_operand(i+1));
    
            if (!IS_CONSTANT_INT(val) && !IS_CONSTANT_FP(val)){
                // args不是常量，phi是基本块第一条指令，不考虑args之前被定植
                phi_use[pre_bb].insert(val);
            }  
        }
    
        if(tmp_use.find(instr) == tmp_use.end()){
            // 左值 instr 定值前无引用
            tmp_def.insert(instr);
        }                       
    }
    ```
## 实验设计

* **常量传播**
    实现思路：

       分为两块进行阐述:

    1. 常量折叠

       这一块比较简单，主要思路是对可能产生常量结果的指令，检查指令的操作数是否均为常量，进而确定指令结果是否为常量，如果是则直接调用`ConstFolder::computeBinary()`或者`ConstantFolder::computeCMP（）`直接生成常量代替原有的值。

    2. 常量传播

       这一块比较困难，主要思路是在进行常量传播时需要考虑当前变量名对应的“状态”，是属于常量状态，还是处于变量状态。我们对常量状态的变量的使用应该向前找到其常量值，直接使用。

    

    相应代码：(节选部分重要代码)

    1. 文件：

       `ConstantPropagation.hpp`: 头文件

       `ConstantPropagation.cpp`: C++文件

    2. 函数：

       `ConstantInt *ConstFolder::computeBinary() `:  两个`Int`常量的常量折叠

       `ConstantFP *ConstFolder::computeBinary()`: 两个`FLOAT`常量的常量折叠

       `ConstantInt *ConstFolder::computeCMP()`: 两个常量比较的常量折叠

       `ConstantFP *cast_constantfp()`: 判断是否为`FLOAT`常量

       `ConstantInt *cast_constantint()`: 判断是否为`Int`常量

       `void ConstPropagation::run()`：常量折叠与传播

       - 普通变量常量传播的设计

         ```c++
         // store指令时记录时候对某个变量有过常数的赋值
         // 右值非常量
         // 擦除对应的表项
         if (r_val_int_constantptr)
         {
             // 保存了一个int常量
             // printf("SAVE A INT!\n");
             std::cout << r_val->get_name();
             ConstantInt_var_list.insert({l_val, r_val_int_constantptr});
         }
         else if (r_val_fp_constantptr)
         {
             ConstantFP_var_list.insert({l_val, r_val_fp_constantptr});
         }
         else
         {
             // 根本不是常量，擦除
             if (ConstantInt_var_list.find(l_val) != ConstantInt_var_list.end())
             {
                 ConstantInt_var_list.erase(l_val);
             }
             if (ConstantFP_var_list.find(l_val) != ConstantFP_var_list.end())
             {
                 ConstantFP_var_list.erase(l_val);
             }
         }
         
         // store指令中将常数变量值直接插入
         if (ConstantInt_var_list.find(r_val) != ConstantInt_var_list.end())
         {
             // 表中显示这是int常量
             // printf("USE A INT!\n");
             auto key = ConstantInt_var_list.find(r_val);
             instr->replace_all_use_with(key->second);
             wait_delete.push_back(instr);
         }
         else if (ConstantFP_var_list.find(r_val) != ConstantFP_var_list.end())
         {
             auto key = ConstantFP_var_list.find(r_val);
             instr->replace_all_use_with(key->second);
             wait_delete.push_back(instr);
         }
         continue;
         ```

       `void DeadCodeDeletion()` 简单的死代码删除

       - 常真常假条件跳转删除

         ```c++
         // 1.常真常假分支删除
             for (auto f : m_->get_functions())
             {
                 for (auto bb : f->get_basic_blocks())
                 {
                     // 对于每个bb观察它的br指令是否为条件跳转
                     auto terminstr = bb->get_terminator();
                     auto brinstr = dynamic_cast<BranchInst *>(terminstr);
         
                     if (brinstr && brinstr->is_cond_br())
                     {
                         auto condition = terminstr->get_operand(0);
                         auto truebb = static_cast<BasicBlock *>(terminstr->get_operand(1));
                         auto falsebb = static_cast<BasicBlock *>(terminstr->get_operand(2));
                         auto condition_value = cast_constantint(condition);
         
                         // condition_value不是常数int
                         if (!condition_value)
                         {
                             // 下一个块
                             continue;
                         }
         
                         auto curr_bb = bb;
                         // 常真常假分支删除
                         auto value = condition_value->get_value();
                         printf("%d\n", value);
                         if (value == 1 || value == 0)
                         {
                             // 后继所有BasicBlock删除该前驱并修改phi指令
                             printf("READY TO DELETE\n");
                             for (auto succbb : curr_bb->get_succ_basic_blocks())
                             {
                                 // 考察所有的后继块前驱，删除跳转不到的后继块的前驱
                                 succbb->remove_pre_basic_block(curr_bb);
         
                                 // 修改 常真跳转的falsebb 和常假跳转的truebb 里面的的 curr_bb phi
                                 if ((value == 1 && succbb != truebb) || (value == 0 && succbb != falsebb))
                                 {
                                     // 修改phi指令，删除
                                     for (auto instr : succbb->get_instructions())
                                     {
                                         if (instr->is_phi())
                                         {
                                             std::cout << succbb->get_name() << std::endl;
                                             for (int i = 1; i < (instr->get_num_operand()); i = i + 2)
                                             {
                                                 if (instr->get_operand(i) == curr_bb)
                                                 {
                                                     // 删掉<value , curr_bb>这一条
                                                     instr->remove_operands(i - 1, i);
                                                 }
                                             }
                                         }
                                     }
                                 }
                             }
         
                             // 删除分支并改为直接跳转到对应的BB
                             bb->delete_instr(brinstr);
                             bb->get_succ_basic_blocks().clear();
         
                             if (value == 1)
                             {
                                 BranchInst::create_br(truebb, bb);
                                 bb->add_succ_basic_block(truebb);
                             }
                             if (value == 0)
                             {
                                 BranchInst::create_br(falsebb, bb);
                                 bb->add_succ_basic_block(falsebb);
                             }
         
                             // std::cout <<bb->get_name() + " OK" << std::endl;
                         }
                     }
                     else
                     {
                         // std::cout << "NOT A COND_BR" <<std::endl;
                     }
                 }
             }
         ```

    优化前后的IR对比（举一个例子）并辅以简单说明：

    `test.cminus:`

    ```
    int x;
    int main(void){
            int a;
            float b;
            x = 1;
            a = 1+2;
            b = (a > x) + a;
            return 0;
    }
    ```

    优化前`test.ll`

    ```
    @x = global i32 0
    declare i32 @input()
    declare void @output(i32)
    declare void @outputFloat(float)
    declare void @neg_idx_except()
    
    define i32 @main() {
    label_entry:
      %op0 = alloca i32
      %op1 = alloca float
      store i32 1, i32* @x
      %op2 = add i32 1, 2
      store i32 %op2, i32* %op0
      %op3 = load i32, i32* %op0
      %op4 = load i32, i32* @x
      %op5 = icmp sgt i32 %op3, %op4
      %op6 = zext i1 %op5 to i32
      %op7 = load i32, i32* %op0
      %op8 = add i32 %op6, %op7
      %op9 = sitofp i32 %op8 to float
      store float %op9, float* %op1
      %op10 = alloca i32
      store i32 0, i32* %op10
      %op11 = load i32, i32* %op10
      ret i32 %op11
    }
    
    ```

    优化后`test.ll`

    ```c
    @x = global i32 0
    declare i32 @input()
    declare void @output(i32)
    declare void @outputFloat(float)
    declare void @neg_idx_except()
    
    define i32 @main() {
    label_entry:
      %op0 = alloca i32					
      %op1 = alloca float				
      store i32 1, i32* @x		
      store i32 3, i32* %op0							 // a = 1 + 2 优化成 a = 3
      store float 0x4010000000000000, float* %op1	   	// b = (a > x) + a 优化成 b = 4;
      %op10 = alloca i32
      store i32 0, i32* %op10
      ret i32 0
    }
    ```

    

* **死代码删除**

    实现思路：

    ​	主要实现了两个简单功能：

    1. 常真常假跳转的删除

       主要思路是对于每个`BasicBlock`，检查其末尾的跳转指令，如果其跳转指令是`Cond_Br`且`Cond`字段为一常量，则将`Cond_Br`替换为`Br`，并修改对应`BasicBlock`间的关系。

    2. 单来源`Phi`指令的删除

       主要思路是对于每个`BasicBlock`，检查开头的`Phi`指令，如果`Phi`指令只有两个操作数，则意味着这个`BasicBlock`只有一个可能的跳转来源，直接删除这个`Phi`指令，并将向前寻找值，替换需要用到`Phi`指令得到值的变量。

       

    相应代码：

    1. 文件:

       `ConstantPropagation.hpp`: 头文件

       `ConstantPropagation.cpp`: C++文件

    2. 函数：

       `void ConstPropagation::DeadCodeDeletion()`: 实现上述两个功能

    

    优化前后的IR对比（举一个例子）并辅以简单说明：

    `test.cminus`:

    ```
    int main(void){
    	int a;
    	a = 1;
    	if(a == 1){
    	 return 0;
    	}	
    	else{
    	 return 1;
    	}
    }
    ```

    优化前`test.ll`:

    ```
    declare i32 @input()
    declare void @output(i32)
    declare void @outputFloat(float)
    declare void @neg_idx_except()
    
    define i32 @main() {
    label_entry:
      %op0 = alloca i32
      store i32 1, i32* %op0
      %op1 = load i32, i32* %op0
      %op2 = icmp eq i32 %op1, 1
      %op3 = zext i1 %op2 to i32
      %op4 = icmp ne i32 %op3, 0
      br i1 %op4, label %label_trueBB0, label %label_falseBB0
    label_trueBB0:                                                ; preds = %label_entry
      %op5 = alloca i32
      store i32 0, i32* %op5
      %op6 = load i32, i32* %op5
      ret i32 %op6
      br label %label_endBB0
    label_falseBB0:                                                ; preds = %label_entry
      %op7 = alloca i32
      store i32 1, i32* %op7
      %op8 = load i32, i32* %op7
      ret i32 %op8
      br label %label_endBB0
    label_endBB0:                                                ; preds = %label_trueBB0, %label_falseBB0
      ret i32 0
      ret i32 0
    }
    ```

    优化后`test.ll`:

    ```c++
    declare i32 @input()
    declare void @output(i32)
    declare void @outputFloat(float)
    declare void @neg_idx_except()
    
    define i32 @main() {
    label_entry:
      %op0 = alloca i32
      store i32 1, i32* %op0
      br label %label_trueBB0	// 只会跳转到label_trueBB0
    label_trueBB0:                                                ; preds = %label_entry
      %op5 = alloca i32
      store i32 0, i32* %op5
      ret i32 0
      br label %label_endBB0
    label_falseBB0:				// label_falseBB0此时不可达，其实可以进行删除，但本次试验没有1去实现
      %op7 = alloca i32
      store i32 1, i32* %op7
      ret i32 1
      br label %label_endBB0
    label_endBB0:                                                ; preds = %label_trueBB0, %label_falseBB0
      ret i32 0
      ret i32 0
    }
    ```

    


* **循环不变式外提**
    实现思路：

    1. 先获取所有函数，通过遍历每个函数来获取每个函数中的所有循环，遍历所有获得的循环，找到每个循环的最内存循环,将最内层的循环入口放入inner_loops中，可以避免重复查找  。

    2. 通过inner_loops，可以得到所有最内层循环，对于每一个最内层循环，由最内层循环访问到最外层循环，然后再访问下一个最内层循环，可以保证对所有循环不变量均进行了代码外提。访问到每一个循环时，先用`labels`记录每一个指令左值，然后对于每一个不是`is_void和is_phi`的指令的操作数，在`labels`中进行查找，如果所有操作数都不在labels中，即所有操作数都在循环外部，则将该不变量存储到`moves`中。

    3. 在访问完一个循环的所有基本块，找到这个循环的所有可外提循环不变量之后，如果moves的大小不为0，则遍历这个循环的所有前继基本块，找到一个在该循环外部的前继基本块，如果这个前继基本块内部有终止指令，则先删除该终止指令，将要外提的循环不变量插入，插入完成后在将该终止指令放回，完成外提操作，再访问下一个循环。

    相应代码：

    1. 获得所有最内层循环

       ```C++
       		InnerloopSet inner_loops;
           auto func_list = m_->get_functions();
           for(auto func: func_list){
               auto loops = loop_searcher.get_loops_in_func(func);
               for(auto loop: loops){
                   auto inner_loop = loop_searcher.get_inner_loop(loop_searcher.get_loop_base(loop));
                   inner_loops.insert(inner_loop);
               }
           }找到一个循环中所有的循环不变量
       ```
    2. 找到一个循环中所有的循环不变量
       ```C++
                       for( auto BB: *in2out_loop){
                           //遍历基本块中的所有语句，记录下label
                           for( auto instr: BB->get_instructions()){
                                   labels.insert(instr);
                           }
                       }
                       //遍历循环中的所有基本块
                       for( auto BB: *in2out_loop){
                           //遍历基本块中的所有语句
                           for(auto instr: BB->get_instructions()){
                               auto is_inv = true;
                               if(!( instr->is_void() || instr->is_phi() )){
                                   for(auto oper: instr->get_operands()){
                                       if(labels.find(oper) != labels.end()){
                                           is_inv = false;
                                           break;
                                       }
                                   }
                                   if(is_inv == true){
                                       labels.erase(instr);
                                       moves.push_back(instr);
                                   }
                               }
                           }
                       }
       ```
    3. 实现代码外提

       ```C++
                       if(moves.size() != 0){
                           for(auto move: moves){
                               auto prebbs = loop_searcher.get_loop_base(in2out_loop)->get_pre_basic_blocks();
                             //找到前继中在循环外部的基本块
                               for (auto prebb : prebbs ){
                                   if(in2out_loop->find(prebb) == in2out_loop->end()){
                                       destina = prebb;
                                   }
                               }
                             //先删除最后一个终止指令，再将需要外提的代码插入，在最后插入原来被删除的终止指令
                               auto term = destina->get_terminator();
                               if(term != nullptr){
                                   destina->delete_instr(term);
                                   move->get_parent()->delete_instr(move);
                                   move->set_parent(destina);
                                   destina->add_instruction(move);
                                   destina->add_instruction(term);
                               }
                           }
                       }
       ```

    优化前后的IR对比（举一个例子）并辅以简单说明：

    `test.cminus`:
    
    ```C
    void main(void){
        int i;
        int j;
        int ret;
    
        i = 1;
        
        while(i<10000)
        {
            j = 0;
            while(j<10000)
            {
                ret = (i*i*i*i*i*i*i*i*i*i)/i/i/i/i/i/i/i/i/i/i;
                j=j+1;
            }
            i=i+1;
        }
    	output(ret);
        return ;
    }
    ```
    
    优化前`test.ll`:
    ```C++
    ; ModuleID = 'cminus'
    source_filename = "../tests/4-ir-opt/testcases/LoopInvHoist/testcase-1.cminus"
    
    declare i32 @input()
    
    declare void @output(i32)
    
    declare void @outputFloat(float)
    
    declare void @neg_idx_except()
    
    define void @main() {
    label_entry:
      %op0 = alloca i32
      %op1 = alloca i32
      %op2 = alloca i32
      store i32 1, i32* %op0
      br label %label3
    label3:                                                ; preds = %label_entry, %label58
      %op4 = load i32, i32* %op0
      %op5 = icmp slt i32 %op4, 10000
      %op6 = zext i1 %op5 to i32
      %op7 = icmp ne i32 %op6, 0
      br i1 %op7, label %label8, label %label9
    label8:                                                ; preds = %label3
      store i32 0, i32* %op1
      br label %label11
    label9:                                                ; preds = %label3
      %op10 = load i32, i32* %op2
      call void @output(i32 %op10)
      ret void
    label11:                                                ; preds = %label8, %label16
      %op12 = load i32, i32* %op1
      %op13 = icmp slt i32 %op12, 10000
      %op14 = zext i1 %op13 to i32
      %op15 = icmp ne i32 %op14, 0
      br i1 %op15, label %label16, label %label58
    label16:                                                ; preds = %label11
      %op17 = load i32, i32* %op0
      %op18 = load i32, i32* %op0
      %op19 = mul i32 %op17, %op18
      %op20 = load i32, i32* %op0
      %op21 = mul i32 %op19, %op20
      %op22 = load i32, i32* %op0
      %op23 = mul i32 %op21, %op22
      %op24 = load i32, i32* %op0
      %op25 = mul i32 %op23, %op24
      %op26 = load i32, i32* %op0
      %op27 = mul i32 %op25, %op26
      %op28 = load i32, i32* %op0
      %op29 = mul i32 %op27, %op28
      %op30 = load i32, i32* %op0
      %op31 = mul i32 %op29, %op30
      %op32 = load i32, i32* %op0
      %op33 = mul i32 %op31, %op32
      %op34 = load i32, i32* %op0
      %op35 = mul i32 %op33, %op34
      %op36 = load i32, i32* %op0
      %op37 = sdiv i32 %op35, %op36
      %op38 = load i32, i32* %op0
      %op39 = sdiv i32 %op37, %op38
      %op40 = load i32, i32* %op0
      %op41 = sdiv i32 %op39, %op40
      %op42 = load i32, i32* %op0
      %op43 = sdiv i32 %op41, %op42
      %op44 = load i32, i32* %op0
      %op45 = sdiv i32 %op43, %op44
      %op46 = load i32, i32* %op0
      %op47 = sdiv i32 %op45, %op46
      %op48 = load i32, i32* %op0
      %op49 = sdiv i32 %op47, %op48
      %op50 = load i32, i32* %op0
      %op51 = sdiv i32 %op49, %op50
      %op52 = load i32, i32* %op0
      %op53 = sdiv i32 %op51, %op52
      %op54 = load i32, i32* %op0
      %op55 = sdiv i32 %op53, %op54
      store i32 %op55, i32* %op2
      %op56 = load i32, i32* %op1
      %op57 = add i32 %op56, 1
      store i32 %op57, i32* %op1
      br label %label11
    label58:                                                ; preds = %label11
      %op59 = load i32, i32* %op0
      %op60 = add i32 %op59, 1
      store i32 %op60, i32* %op0
      br label %label3
    }
    ```

    优化后`test.ll`:
    
    ```C++
    declare i32 @input()
    
    declare void @output(i32)
    
    declare void @outputFloat(float)
    
    declare void @neg_idx_except()
    
    define void @main() {
    label_entry:
      %op0 = alloca i32
      %op1 = alloca i32
      %op2 = alloca i32
      store i32 1, i32* %op0
      %op12 = load i32, i32* %op1
      %op13 = icmp slt i32 %op12, 10000
      %op14 = zext i1 %op13 to i32
      %op15 = icmp ne i32 %op14, 0
      %op17 = load i32, i32* %op0
      %op18 = load i32, i32* %op0
      %op19 = mul i32 %op17, %op18
      %op20 = load i32, i32* %op0
      %op21 = mul i32 %op19, %op20
      %op22 = load i32, i32* %op0
      %op23 = mul i32 %op21, %op22
      %op24 = load i32, i32* %op0
      %op25 = mul i32 %op23, %op24
      %op26 = load i32, i32* %op0
      %op27 = mul i32 %op25, %op26
      %op28 = load i32, i32* %op0
      %op29 = mul i32 %op27, %op28
      %op30 = load i32, i32* %op0
      %op31 = mul i32 %op29, %op30
      %op32 = load i32, i32* %op0
      %op33 = mul i32 %op31, %op32
      %op34 = load i32, i32* %op0
      %op35 = mul i32 %op33, %op34
      %op36 = load i32, i32* %op0
      %op37 = sdiv i32 %op35, %op36
      %op38 = load i32, i32* %op0
      %op39 = sdiv i32 %op37, %op38
      %op40 = load i32, i32* %op0
      %op41 = sdiv i32 %op39, %op40
      %op42 = load i32, i32* %op0
      %op43 = sdiv i32 %op41, %op42
      %op44 = load i32, i32* %op0
      %op45 = sdiv i32 %op43, %op44
      %op46 = load i32, i32* %op0
      %op47 = sdiv i32 %op45, %op46
      %op48 = load i32, i32* %op0
      %op49 = sdiv i32 %op47, %op48
      %op50 = load i32, i32* %op0
      %op51 = sdiv i32 %op49, %op50
      %op52 = load i32, i32* %op0
      %op53 = sdiv i32 %op51, %op52
      %op54 = load i32, i32* %op0
      %op55 = sdiv i32 %op53, %op54
      %op56 = load i32, i32* %op1
      %op57 = add i32 %op56, 1
      %op4 = load i32, i32* %op0
      %op5 = icmp slt i32 %op4, 10000
      %op6 = zext i1 %op5 to i32
      %op7 = icmp ne i32 %op6, 0
      %op59 = load i32, i32* %op0
      %op60 = add i32 %op59, 1       //以上新增的指令均为循环不变量外提的结果
      br label %label3               //br指令在基本块最后位置不变
    label3:                                                ; preds = %label_entry, %label58
      br i1 %op7, label %label8, label %label9
    label8:                                                ; preds = %label3
      store i32 0, i32* %op1
      br label %label11
    label9:                                                ; preds = %label3
      %op10 = load i32, i32* %op2
      call void @output(i32 %op10)
      ret void
    label11:                                                ; preds = %label8, %label16
      br i1 %op15, label %label16, label %label58
    label16:                                                ; preds = %label11
      store i32 %op55, i32* %op2
      store i32 %op57, i32* %op1
      br label %label11
    label58:                                                ; preds = %label11
      store i32 %op60, i32* %op0
      br label %label3
    }
    ```



* 活跃变量分析

    实现思路：

    1. 遍历基本块，遍历每个基本块的指令，根据指令的类型，分析出指令包含的左值、右值。左值一定为变量，故只需判断是否`定值前无引用`，如果是则加入 `def` 中；右值需要判断是否为`常数`以及是否`引用前无定值`，如果是变量且引用前无定值则加入 `use`。最终得到 `live_use` 和 `live_def`，用 `live_use` 初始化`live_in`。
    2. 再次遍历基本块（没有进行逆序遍历），先根据 `live_in` 以及 `phi_use` 得到每个基本块的 `live_out`，再根据 `live_out`、`live_def` 以及 `live_in` 得到每个基本块的 `live_in`。如果有基本块的 `live_in` 发生改变，则清空 `live_out`，重复步骤2。

    相应代码：
    
    1. 计算 `use`、`def`，以 `二目运算指令及比较指令` 为例
    
       ```cpp
       if(instr->isBinary() || instr->is_cmp() || instr->is_fcmp()){ //一左两右
           auto op1 = instr->get_operand(0);
           auto op2 = instr->get_operand(1);
       
           if (!IS_CONSTANT_INT(op1) && !IS_CONSTANT_FP(op1)
               && tmp_def.find(op1)==tmp_def.end()){
               // 右值 op1 不是常量，且引用前无定值
               tmp_use.insert(op1);
           }
       
           if (!IS_CONSTANT_INT(op2) && !IS_CONSTANT_FP(op2)
               && tmp_def.find(op2) == tmp_def.end()){
               // 右值 op2 不是常量，且引用前无定值
               tmp_use.insert(op2);
           }
       
           if(tmp_use.find(instr) == tmp_use.end()){
               // 左值 instr 定值前无引用
               tmp_def.insert(instr);
           }
       }
       ```
    
       每遍历一个基本块，将结果保存在 `live_use` 和 `live_def`。
    
       ```cpp
       live_use.insert({bb, tmp_use});
       live_def.insert({bb, tmp_def});
       ```
    
    2. 计算 `live_in` 和 `live_out`
    
       ```cpp
       // 迭代计算每个基本块的 live_in live_out，没有使用逆序访问
       bool isChanged = true;
       //用 live_use 初始化 live_in
       live_in = live_use; 
       while(isChanged){
           isChanged = false;
           // 每次清空 live_out，不清空 live_in
           live_out.clear();
       
           for(auto bb : func_->get_basic_blocks()){
               tmp_in.clear();
               tmp_out.clear();
       
               // live_out
               for(auto succ : bb->get_succ_basic_blocks()){
                   // 后继，迭代 live_out
                   auto succIn= live_in[succ];
                   live_out[bb].insert(succIn.begin(), succIn.end());
               }
               live_out[bb].insert(phi_use[bb].begin(), phi_use[bb].end());
       
               // tmp_in = live_use U (live_out - live_def)
               // tmp_out = live_out - live_def
               tmp_out = live_out[bb];
               for(auto& iter : live_def[bb]){
                   if(tmp_out.find(iter) != tmp_out.end())
                       tmp_out.erase(iter);
               }
               tmp_in.insert(tmp_out.begin(), tmp_out.end());
               tmp_in.insert(live_use[bb].begin(), live_use[bb].end());
       
               if(live_in[bb] != tmp_in){
                   // 更新该基本块 live_in
                   live_in[bb].clear();
                   isChanged = true;
                   live_in[bb].insert(tmp_in.begin(), tmp_in.end());
               }
           }
       }
       ```

### 实验总结

此次实验有什么收获

对 C++ 的 STL 有了更好的掌握。对课堂上理论讲解的三种优化进行了具体实现，有了更形象的认识。

### 实验反馈 （可选 不会评分）

对本次实验的建议

### 组间交流 （可选）

本次实验和哪些组（记录组长学号）交流了哪一部分信息
