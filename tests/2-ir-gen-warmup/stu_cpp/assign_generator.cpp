#include "BasicBlock.h"
#include "Constant.h"
#include "Function.h"
#include "IRBuilder.h"
#include "Module.h"
#include "Type.h"

#include <iostream>
#include <memory>

#ifdef DEBUG
#define DEBUG_OUTPUT std::cout << "Here" << std::endl;
#else
#define DEBUG_OUTPUT
#endif

#define CONST_INT(num) \
        ConstantInt::get(num,module)
    
#define CONST_FP(num)   \
        ConstantFP::get(num,module)

int main(){
    auto module = new Module("Assign.cminus");
    auto builder = new IRBuilder(nullptr, module);
    Type *Int32Type = Type::get_int32_type(module);

    // main函数
    std::vector<Type *> Ints(0);
    auto mainFunTy = FunctionType::get(Int32Type,Ints);
    auto mainFun = Function::create(mainFunTy,"main",module);

    // 基本块
    auto bb = BasicBlock::create(module,"entry",mainFun);
    builder->set_insert_point(bb);

    auto ArrayAlloca = builder->create_alloca(ArrayType::get(Int32Type,10));
    auto retAlloca = builder->create_alloca(Int32Type);

    auto A0GEP = builder->create_gep(ArrayAlloca,{CONST_INT(0),CONST_INT(0)});
    builder->create_store(CONST_INT(10),A0GEP);
    auto A1GEP = builder->create_gep(ArrayAlloca,{CONST_INT(0),CONST_INT(1)});
    auto A0Load = builder->create_load(A0GEP);
    auto mul = builder->create_imul(CONST_INT(2),A0Load);
    builder->create_store(mul,A1GEP);

    auto A1Load = builder->create_load(A1GEP);
    builder->create_store(A1Load,retAlloca);

    auto retLoad = builder->create_load(retAlloca);
    builder->create_ret(retLoad);

    std::cout << module->print();
    delete module;
    return 0;
}
