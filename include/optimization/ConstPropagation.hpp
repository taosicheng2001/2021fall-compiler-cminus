#ifndef CONSTPROPAGATION_HPP
#define CONSTPROPAGATION_HPP
#include "PassManager.hpp"
#include "Constant.h"
#include "Instruction.h"
#include "Module.h"

#include "Value.h"
#include "IRBuilder.h"
#include <vector>
#include <stack>
#include <unordered_map>

// tips: 用来判断value是否为ConstantFP/ConstantInt
ConstantFP* cast_constantfp(Value *value);
ConstantInt* cast_constantint(Value *value);


// tips: ConstFloder类
class ConstFolder
{
public:
    ConstFolder(Module *m) : module_(m) {}
    // tips: 两个ConstantInt常量合成一个ConstantInt
    ConstantInt *computeBinary(
        Instruction::OpID addop,
        ConstantInt *value1,
        ConstantInt *value2);
    // tips: 两个ConstantFP常量合成一个ConstantFP
    ConstantFP *computeBinary(
        Instruction::OpID addop,
        ConstantFP *value1,
        ConstantFP *value2);

    // tips: 两个常量比较出一个ConstantInt
    ConstantInt *computeCMP(
        int  relop,
        Value *value1,
        Value *value2);
    
    //...
private:
    Module *module_;
};

class ConstPropagation : public Pass
{
public:
    ConstPropagation(Module *m) : Pass(m)
    {
        Folder = new ConstFolder(m);
        INT = Type::get_int32_type(m);
        FLOAT = Type::get_float_type(m);
        INT_ZERO = ConstantInt::get(0,m);
        FP_ZERO = ConstantFP::get(0,m);
        TRUE = ConstantInt::get(true,m);
        FALSE = ConstantInt::get(false,m);
        m_ = m;
    }
    void run();
    void DeadCodeDeletion(Module* m_);
    
private:
    Module* m_;
    ConstFolder *Folder;
    Type* INT;
    Type* FLOAT;
    ConstantInt* TRUE;
    ConstantInt* FALSE;
    ConstantInt* INT_ZERO;
    ConstantFP* FP_ZERO;
};

#endif
