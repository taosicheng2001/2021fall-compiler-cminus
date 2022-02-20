#include <algorithm>
#include "logging.hpp"
#include "LoopSearch.hpp"
#include "LoopInvHoist.hpp"

void LoopInvHoist::run()
{
    // 先通过LoopSearch获取循环的相关信息
    LoopSearch loop_searcher(m_, false);
    loop_searcher.run();
    // 接下来由你来补充啦！
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
                LabelSet labels;
                MoveSet moves;
                BasicBlock *destina;
                //遍历循环中的所有基本块
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
                //找到这个循环所有的可外提代码后，进行代码外提
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
                in2out_loop = loop_searcher.get_parent_loop(in2out_loop);
            }
        }
}

