#ifndef ACTIVEVARS_HPP
#define ACTIVEVARS_HPP
#include "PassManager.hpp"
#include "Constant.h"
#include "Instruction.h"
#include "Module.h"

#include "Value.h"
#include "IRBuilder.h"
#include <vector>
#include <stack>
#include <unordered_map>
#include <map>
#include <queue>
#include <fstream>

class ActiveVars : public Pass
{
public:
    ActiveVars(Module *m) : Pass(m) {}
    void run();
    std::string print();
private:
    Function *func_;

    // 基本块入口出口的活跃变量
    std::map<BasicBlock *, std::set<Value *>> live_in, live_out;
    // 基本块内部注销和生成的活跃变量
    std::map<BasicBlock *, std::set<Value *>> live_use, live_def;
    
    // 临时计算使用
    std::set<Value *> tmp_use, tmp_def, tmp_in, tmp_out;
    
    // 记录前驱节点对应phi指令的活跃变量
    std::map<BasicBlock *, std::set<Value *>> phi_use;
};

#endif
