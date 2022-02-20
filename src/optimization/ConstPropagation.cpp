#include "ConstPropagation.hpp"
#include "logging.hpp"

// int addop int
ConstantInt *ConstFolder::computeBinary(
    Instruction::OpID addop, //四则运算操作
    ConstantInt *value1,
    ConstantInt *value2)
{
    int c_value1 = value1->get_value();
    int c_value2 = value2->get_value();

    // 加减乘除 c_value1 addop c_value_2
    switch (addop)
    {
    case Instruction::add:
        return ConstantInt::get(c_value1 + c_value2, module_);
        break;
    case Instruction::sub:
        return ConstantInt::get(c_value1 - c_value2, module_);
        break;
    case Instruction::mul:
        return ConstantInt::get(c_value1 * c_value2, module_);
        break;
    case Instruction::sdiv:
        return ConstantInt::get((int)(c_value1 / c_value2), module_);
        break;
    // 出口
    default:
        return nullptr;
        break;
    }
}

// fp addop fp
ConstantFP *ConstFolder::computeBinary(
    Instruction::OpID addop, //四则运算操作
    ConstantFP *value1,
    ConstantFP *value2)
{
    float c_value1 = value1->get_value();
    float c_value2 = value2->get_value();

    // 加减乘除 c_value1 addop c_value_2
    switch (addop)
    {
    case Instruction::fadd:
        return ConstantFP::get(c_value1 + c_value2, module_);
        break;
    case Instruction::fsub:
        return ConstantFP::get(c_value1 - c_value2, module_);
        break;
    case Instruction::fmul:
        return ConstantFP::get(c_value1 * c_value2, module_);
        break;
    case Instruction::fdiv:
        return ConstantFP::get(c_value1 / c_value2, module_);
        break;
    // 出口
    default:
        return nullptr;
        break;
    }
}

// 两个常数比较出一个ConstantInt
ConstantInt *ConstFolder::computeCMP(
    int relop,
    Value *value1,
    Value *value2)
{
    ConstantInt *value1_int_ptr = nullptr;
    ConstantInt *value2_int_ptr = nullptr;
    ConstantFP *value1_fp_ptr = nullptr;
    ConstantFP *value2_fp_ptr = nullptr;

    value1_int_ptr = cast_constantint(value1);
    if (!value1_int_ptr)
    {
        value1_fp_ptr = cast_constantfp(value1);
    }
    value2_int_ptr = cast_constantint(value2);
    if (!value2_int_ptr)
    {
        value2_fp_ptr = cast_constantfp(value2);
    }

    auto c_value1 = (!value1_int_ptr) ? value1_fp_ptr->get_value() : value1_int_ptr->get_value();
    auto c_value2 = (!value2_int_ptr) ? value2_fp_ptr->get_value() : value2_int_ptr->get_value();

    switch (relop)
    {
    case CmpInst::EQ:
        return ConstantInt::get((c_value1 == c_value2), module_);
        break;
    case CmpInst::NE:
        return ConstantInt::get((c_value1 != c_value2), module_);
        break;
    case CmpInst::GT:
        return ConstantInt::get((c_value1 > c_value2), module_);
        break;
    case CmpInst::GE:
        return ConstantInt::get((c_value1 >= c_value2), module_);
        break;
    case CmpInst::LT:
        return ConstantInt::get((c_value1 < c_value2), module_);
        break;
    case CmpInst::LE:
        return ConstantInt::get((c_value1 <= c_value2), module_);
        break;
    // 出口
    default:
        return nullptr;
        break;
    }
}

// 用来判断value是否为ConstantFP，如果不是则会返回nullptr
ConstantFP *cast_constantfp(Value *value)
{
    auto constant_fp_ptr = dynamic_cast<ConstantFP *>(value);
    if (constant_fp_ptr)
    {
        return constant_fp_ptr;
    }
    else
    {
        return nullptr;
    }
}
// 用来判断value是否为ConstantInt，如果不是则会返回nullptr
ConstantInt *cast_constantint(Value *value)
{
    auto constant_int_ptr = dynamic_cast<ConstantInt *>(value);
    if (constant_int_ptr)
    {
        return constant_int_ptr;
    }
    else
    {
        return nullptr;
    }
}

// 死代码删除
void ConstPropagation::DeadCodeDeletion(Module *m_)
{
    // 只实现两个部分
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

    // 2.单一入口phi指令删除
    for (auto f : m_->get_functions())
    {
        std::vector<Instruction *> wait_delete;
        for (auto bb : f->get_basic_blocks())
        {
            for (auto instr : bb->get_instructions())
            {
                if (instr->is_phi())
                {
                    auto operand_num = instr->get_num_operand();
                    if (operand_num > 2)
                    {
                        // 有两个及以上的入口
                        continue;
                    }

                    // 只有一个入口 直接把前面的结果拿回来
                    auto val = instr->get_operand(0);
                    instr->replace_all_use_with(val);
                    wait_delete.push_back(instr);
                }
            }
            for (auto instr : wait_delete)
            {
                bb->delete_instr(instr);
            }
            wait_delete.clear();
        }
    }
}

void ConstPropagation::run()
{
    // 从这里开始吧！
    //  1.遍历所有函数
    for (auto f : m_->get_functions())
    {
        // 在某个Function内部
        auto func_ = f;

        for (auto bb : func_->get_basic_blocks())
        {
            // 保存需要删除的<instr>
            std::vector<Instruction *> wait_delete;

            // 用于块内常量传播
            std::map<Value *, ConstantInt *> ConstantInt_var_list;
            std::map<Value *, ConstantFP *> ConstantFP_var_list;

            // 只需要考虑块内被定值的全局变量
            std::map<GlobalVariable *, ConstantInt *> global_int_list;
            std::map<GlobalVariable *, ConstantFP *> global_fp_list;
            for (auto instr : bb->get_instructions())
            {
                // 2.四则运算指令
                if (instr->isBinary())
                {
                    auto l_val = instr->get_operand(0);
                    auto r_val = instr->get_operand(1);

                    // 检查l_val是否为常量
                    auto l_int_constantptr = cast_constantint(l_val);
                    auto l_fp_constantptr = cast_constantfp(l_val);
                    // 检查r_val是否为常量
                    auto r_int_constantptr = cast_constantint(r_val);
                    auto r_fp_constantptr = cast_constantfp(r_val);

                    // 类型不同的两个变量会在运算前转化，所以此处一定是两个一样类型的变量
                    if (l_int_constantptr && r_int_constantptr)
                    {
                        // 两个int合并
                        // printf("%d %d %d!\n",l_int_constantptr->get_value(),instr->get_instr_type(),r_int_constantptr->get_value());
                        auto result = Folder->computeBinary(instr->get_instr_type(), l_int_constantptr, r_int_constantptr);
                        // printf("result:%d\n",result->get_value());
                        instr->replace_all_use_with(result);
                        wait_delete.push_back(instr);
                    }
                    else if (l_fp_constantptr && r_fp_constantptr)
                    {
                        // 两个fp合并
                        // printf("FP OP FP!\n");
                        auto result = Folder->computeBinary(instr->get_instr_type(), l_fp_constantptr, r_fp_constantptr);
                        instr->replace_all_use_with(result);
                        wait_delete.push_back(instr);
                    }
                    else
                    {
                        // 不是两个常量，不进行合并，跳过
                        continue;
                    }
                }

                // 2.比较指令
                if (instr->is_cmp() || instr->is_fcmp())
                {
                    auto l_val = instr->get_operand(0);
                    auto r_val = instr->get_operand(1);

                    // 检查l_val是否为常量
                    auto l_int_constantptr = cast_constantint(l_val);
                    auto l_fp_constantptr = cast_constantfp(l_val);
                    // 检查r_val是否为常量
                    auto r_int_constantptr = cast_constantint(r_val);
                    auto r_fp_constantptr = cast_constantfp(r_val);

                    // 这里合并了cmp和fcmp,因为CmpOP是一致的
                    if (l_fp_constantptr && r_fp_constantptr)
                    {
                        // 两个fp比较
                        auto instr_ptr = static_cast<FCmpInst *>(instr);
                        auto result = Folder->computeCMP(instr_ptr->get_cmp_op(), l_val, r_val);
                        instr->replace_all_use_with(result);
                        wait_delete.push_back(instr);
                    }
                    else if (l_int_constantptr && r_int_constantptr)
                    {
                        // 两个int比较
                        auto instr_ptr = static_cast<FCmpInst *>(instr);
                        auto result = Folder->computeCMP(instr_ptr->get_cmp_op(), l_val, r_val);
                        instr->replace_all_use_with(result);
                        wait_delete.push_back(instr);
                    }
                    else
                    {
                        // 不是两个常量，跳过
                        continue;
                    }
                }

                // 2. 类型转化指令
                if (instr->is_fp2si() || instr->is_si2fp())
                {
                    auto val = instr->get_operand(0);
                    auto val_int_constantptr = cast_constantint(val);
                    auto val_fp_constantptr = cast_constantfp(val);
                    if (val_int_constantptr)
                    {
                        // 这原来是常量int，要转成常量fp
                        float val = val_int_constantptr->get_value();
                        auto result = ConstantFP::get(val, m_);
                        instr->replace_all_use_with(result);
                        wait_delete.push_back(instr);
                    }
                    else if (val_fp_constantptr)
                    {
                        // 原来是fp,要转成int
                        int val = val_fp_constantptr->get_value();
                        auto result = ConstantInt::get(val, m_);
                        instr->replace_all_use_with(result);
                        wait_delete.push_back(instr);
                    }
                    else
                    {
                        // 其他情况，跳过
                        continue;
                    }
                }

                // 2.赋值指令
                if (instr->is_store())
                {
                    // store r_val into l_val
                    auto l_val = static_cast<StoreInst *>(instr)->get_lval();
                    auto r_val = static_cast<StoreInst *>(instr)->get_rval();

                    // 分左值是global和不是global进行
                    auto global_val = dynamic_cast<GlobalVariable *>(l_val);
                    if (global_val)
                    {
                        // 左值为global_val
                        // 此时需要考虑右值是否为常量
                        auto r_val_int_constantptr = cast_constantint(r_val);
                        auto r_val_fp_constantptr = cast_constantfp(r_val);
                        if (r_val_int_constantptr)
                        {
                            // 右值为int常量
                            if (global_int_list.find(global_val) == global_int_list.end())
                            {
                                // 表中没有这一项
                                // 插入记录表
                                // printf("INSERT A RECORD\n");
                                global_int_list.insert({global_val, r_val_int_constantptr});
                            }
                            else
                            {
                                // 更新记录表
                                // printf("UPDATA A RECORD\n");
                                global_int_list[global_val] = r_val_int_constantptr;
                            }
                        }
                        else if (r_val_fp_constantptr)
                        {
                            // 右值为fp常量
                            if (global_fp_list.find(global_val) == global_fp_list.end())
                            {
                                // 表中没有这一项
                                // 插入
                                global_fp_list.insert({global_val, r_val_fp_constantptr});
                            }
                            else
                            {
                                // 更新记录表
                                global_fp_list[global_val] = r_val_fp_constantptr;
                            }
                        }
                        else
                        {
                            // 右值非常量
                            // 擦除对应的表项
                            if (r_val->get_type() == INT)
                            {
                                // 是int类型
                                if (global_int_list.find(global_val) != global_int_list.end())
                                {
                                    // 表中有，删去
                                    global_int_list.erase(global_int_list.find(global_val));
                                }
                            }
                            else if (r_val->get_type() == FLOAT)
                            {
                                // 是float类型
                                if (global_fp_list.find(global_val) != global_fp_list.end())
                                {
                                    // 表中有，删去
                                    global_fp_list.erase(global_fp_list.find(global_val));
                                }
                            }
                            else
                            {
                                // 其他类型的全局变量不处理
                                continue;
                            }
                        }
                    }
                    else
                    {
                        //正常变量
                        // store时候可以发现是否为常量
                        auto r_val_int_constantptr = cast_constantint(r_val);
                        auto r_val_fp_constantptr = cast_constantfp(r_val);

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
                        continue;
                    }
                }

                // 2.load指令
                if (instr->is_load())
                {
                    // 对于普通变量，且为常数，则应该进行常量传播
                    // 只需要考虑load的是global的情况，即 load @global_a  to global_a
                    auto val = static_cast<LoadInst *>(instr);
                    auto global_val = dynamic_cast<GlobalVariable *>(static_cast<LoadInst *>(instr)->get_operand(0));
                    // 是对global的load
                    if (global_val)
                    {
                        // printf("A GLOBAL\n");
                        if (global_val->get_operand(0)->get_type() == INT)
                        {
                            // load int global
                            // printf("A INT GLOBAL\n");
                            if (global_int_list.find(global_val) != global_int_list.end())
                            {
                                // 表中存在对应的int global常量
                                // printf("USE A RECORD!\n");
                                instr->replace_all_use_with(global_int_list[global_val]);
                                wait_delete.push_back(instr);
                            }
                        }
                        if (global_val->get_operand(0)->get_type() == FLOAT)
                        {
                            // load float global
                            if (global_fp_list.find(global_val) != global_fp_list.end())
                            {
                                // 存在对应的float global常量
                                instr->replace_all_use_with(global_fp_list[global_val]);
                                wait_delete.push_back(instr);
                            }
                        }
                        // printf("HOW");
                    }
                    else
                    {
                        // 非全局变量，跳过
                        // printf("NOT A GLOBAL!\n");

                        auto r_val = static_cast<LoadInst *>(instr)->get_operand(0);

                        auto v = r_val->get_name();
                        std::cout << v;

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
                    }
                }

                // 2.零扩展指令
                if (instr->is_zext())
                {
                    auto val = cast_constantint(instr->get_operand(0));
                    if (val)
                    {
                        int iteral_val = val->get_value();
                        auto result = ConstantInt::get(iteral_val, m_);
                        instr->replace_all_use_with(result);
                        wait_delete.push_back(instr);
                    }
                }
            }

            // 4.用合并后的value替换原value，只考虑做块内的替换
            for (auto instr : wait_delete)
            {
                bb->delete_instr(instr);
            }
        }
    }

    printf("ConstPropagation Complete!\n");
    std::cout << "DeadCodeDeletion Start" << std::endl;

    // 死代码删除
    DeadCodeDeletion(m_);
}
