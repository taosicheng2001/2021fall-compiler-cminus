#include "BasicBlock.h"
#include "Constant.h"
#include "Function.h"
#include "IRBuilder.h"
#include "Module.h"
#include "Type.h"

#include <iostream>
#include <memory>

#ifdef DEBUG  // 用于调试信息,大家可以在编译过程中通过" -DDEBUG"来开启这一选项
#define DEBUG_OUTPUT std::cout << __LINE__ << std::endl;  // 输出行号的简单示例
#else
#define DEBUG_OUTPUT
#endif

#define CONST_INT(num) \
    ConstantInt::get(num, module)

#define CONST_FP(num) \
    ConstantFP::get(num, module) // 得到常数值的表示,方便后面多次用到

int main(){
    
    auto module = new Module("WhileFunction.cminus");
    auto builder = new IRBuilder(nullptr,module);
    Type *Int32Type = Type::get_int32_type(module);

    auto mainFun = Function::create(FunctionType::get(Int32Type, {}),"main",module);
    auto bb = BasicBlock::create(module,"entry",mainFun);
    builder->set_insert_point(bb);

    auto retAlloca = builder->create_alloca(Int32Type);
    auto aAlloca = builder->create_alloca(Int32Type);
    auto iAlloca = builder->create_alloca(Int32Type);

    builder->create_store(CONST_INT(10),aAlloca);
    builder->create_store(CONST_INT(0),iAlloca);

    // pre-definition of BB
    auto cmpBB = BasicBlock::create(module,"",mainFun);
    auto trueBB = BasicBlock::create(module,"",mainFun);
    auto retBB = BasicBlock::create(module,"",mainFun);

    auto br = builder->create_br(cmpBB); 
    
    // cmpBB 
    builder->set_insert_point(cmpBB);
    auto iLoad = builder->create_load(iAlloca);
    auto icmp = builder->create_icmp_lt(iLoad,CONST_INT(10));
    builder->create_cond_br(icmp,trueBB,retBB);

    // trueBB
    builder->set_insert_point(trueBB);
    iLoad = builder->create_load(iAlloca);
    auto add = builder->create_iadd(iLoad,CONST_INT(1));
    builder->create_store(add,iAlloca);

    iLoad = builder->create_load(iAlloca);
    auto aLoad = builder->create_load(aAlloca);
    add = builder->create_iadd(iLoad,aLoad);
    builder->create_store(add,aAlloca);
    builder->create_br(cmpBB);

    // retBB
    builder->set_insert_point(retBB);
    aLoad = builder->create_load(aAlloca);
    builder->create_store(aLoad,retAlloca);
    auto retLoad = builder->create_load(retAlloca);
    builder->create_ret(retLoad);
    
    std::cout << module->print();
    delete module;
    return 0;
}