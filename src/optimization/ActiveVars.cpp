#include "ActiveVars.hpp"
#include "logging.hpp"

#define IS_CONSTANT_INT(_val) dynamic_cast<ConstantInt *>(_val)
#define IS_CONSTANT_FP(_val) dynamic_cast<ConstantFP *>(_val)

void ActiveVars::run()
{
    std::ofstream output_active_vars;
    output_active_vars.open("active_vars.json", std::ios::out);
    output_active_vars << "[";
    // 以函数为单位分析活跃变量
    for (auto &func : this->m_->get_functions()) {
        if (func->get_basic_blocks().empty()) {
            continue;
        }
        else
        {
            func_ = func;  

            func_->set_instr_name();   // Tao'S Comfuse

            // 此时清空所有基本块内部信息
            live_in.clear();
            live_out.clear();
            
            // 在此分析 func_ 的每个bb块的活跃变量，并存储在 live_in live_out 结构内

            live_use.clear();
            live_def.clear();
            phi_use.clear();

            // 计算每个基本块的 live_use, live_def
            // 复合计算每个语句的结果，得到基本块的use和def
            for(auto bb : func_->get_basic_blocks()){
                tmp_use.clear();
                tmp_def.clear();
                
                // 遍历指令
                for(auto instr : bb->get_instructions()){

                    // Binary and compare
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
                    
                    // Memory operators
                    else if(instr->is_alloca()){ // 一个左值
                        if(tmp_use.find(instr) == tmp_use.end()){
                            // 左值 instr 定值前无引用
                            tmp_def.insert(instr);
                        }
                    }
                    else if(instr->is_load()){ // 一左一右
                        auto ptr = instr->get_operand(0);
                        
                        if (!IS_CONSTANT_INT(ptr) && !IS_CONSTANT_FP(ptr)
                            && tmp_def.find(ptr)==tmp_def.end()){
                            // 右值 ptr 不是常量，且引用前无定值
                            tmp_use.insert(ptr);
                        }

                        if(tmp_use.find(instr) == tmp_use.end()){
                            // 左值 instr 定值前无引用
                            tmp_def.insert(instr);
                        }
                    }
                    else if(instr->is_store()){ // 两个右值
                        auto val = instr->get_operand(0);
                        auto ptr = instr->get_operand(1);
                        
                        if (!IS_CONSTANT_INT(val) && !IS_CONSTANT_FP(val)
                            && tmp_def.find(val)==tmp_def.end()){
                            // 右值 val 不是常量，且引用前无定值
                            tmp_use.insert(val);
                        }

                        if (!IS_CONSTANT_INT(ptr) && !IS_CONSTANT_FP(ptr)
                            && tmp_def.find(ptr)==tmp_def.end()){
                            // 右值 ptr 不是常量，且引用前无定值
                            tmp_use.insert(ptr);
                        }
                    }

                    // CastInst
                    else if(instr->is_zext() || instr->is_fp2si() || instr->is_si2fp()){ // 一左一右
                        auto val = instr->get_operand(0);
                        
                        if (!IS_CONSTANT_INT(val) && !IS_CONSTANT_FP(val)
                            && tmp_def.find(val)==tmp_def.end()){
                            // 右值 val 不是常量，且引用前无定值
                            tmp_use.insert(val);
                        }

                        if(tmp_use.find(instr) == tmp_use.end()){
                            // 左值 instr 定值前无引用
                            tmp_def.insert(instr);
                        }
                    }

                    // Terminator Instructions
                    else if(instr->is_ret()){ // 两种格式
                        // ret void 不含变量
                        // ret <type> <value> 一个右值
                        if(!static_cast<ReturnInst *>(instr)->is_void_ret()){
                            auto val = instr->get_operand(0);
                            
                            if (!IS_CONSTANT_INT(val) && !IS_CONSTANT_FP(val)
                                && tmp_def.find(val)==tmp_def.end()){
                                // 右值 val 不是常量，且引用前无定值
                                tmp_use.insert(val);
                            }
                        }
                    }
                    else if(instr->is_br()){ // 两种格式
                        // 无条件分支不含变量
                        // 条件分支有一个右值
                        if(static_cast<BranchInst *>(instr)->is_cond_br()){
                            auto cond = instr->get_operand(0);

                            if (!IS_CONSTANT_INT(cond) && !IS_CONSTANT_FP(cond)
                                && tmp_def.find(cond)==tmp_def.end()){
                                // 右值 cond 不是常量，且引用前无定值
                                tmp_use.insert(cond);
                            }
                        }
                    }

                    // Other operators
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
                    else if(instr->is_gep()){ // 一个左值，右值 一个基地址，偏移量不定
                        auto _num = instr->get_num_operand();
                        for(int i = 0; i < _num; i++){
                            auto idx = instr->get_operand(i);

                            if (!IS_CONSTANT_INT(idx) && !IS_CONSTANT_FP(idx)
                                && tmp_def.find(idx)==tmp_def.end()){
                                // 右值 args 不是常量，且引用前无定值
                                tmp_use.insert(idx);
                            }  
                        }

                        if(tmp_use.find(instr) == tmp_use.end()){
                            // 左值 instr 定值前无引用
                            tmp_def.insert(instr);
                        }
                    }
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
                }
                live_use.insert({bb, tmp_use});
                live_def.insert({bb, tmp_def});
            }//end

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

                    // live_out = 后继的live_in的并集 + phi的BB
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
            

            output_active_vars << print();
            output_active_vars << ",";
        }
    }
    output_active_vars << "]";
    output_active_vars.close();
    return ;
}

std::string ActiveVars::print()
{
    std::string active_vars;
    active_vars +=  "{\n";
    active_vars +=  "\"function\": \"";
    active_vars +=  func_->get_name();
    active_vars +=  "\",\n";

    active_vars +=  "\"live_in\": {\n";
    for (auto &p : live_in) {
        if (p.second.size() == 0) {
            continue;
        } else {
            active_vars +=  "  \"";
            active_vars +=  p.first->get_name();
            active_vars +=  "\": [" ;
            for (auto &v : p.second) {
                active_vars +=  "\"%";
                active_vars +=  v->get_name();
                active_vars +=  "\",";
            }
            active_vars += "]" ;
            active_vars += ",\n";   
        }
    }
    active_vars += "\n";
    active_vars +=  "    },\n";
    
    active_vars +=  "\"live_out\": {\n";
    for (auto &p : live_out) {
        if (p.second.size() == 0) {
            continue;
        } else {
            active_vars +=  "  \"";
            active_vars +=  p.first->get_name();
            active_vars +=  "\": [" ;
            for (auto &v : p.second) {
                active_vars +=  "\"%";
                active_vars +=  v->get_name();
                active_vars +=  "\",";
            }
            active_vars += "]";
            active_vars += ",\n";
        }
    }
    active_vars += "\n";
    active_vars += "    }\n";

    active_vars += "}\n";
    active_vars += "\n";
    return active_vars;
}