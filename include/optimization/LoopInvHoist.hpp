#pragma once

#include <unordered_map>
#include <unordered_set>
#include "PassManager.hpp"
#include "Module.h"
#include "Function.h"
#include "BasicBlock.h"

using InnerloopSet = std::unordered_set<BBset_t *>;
using LabelSet = std::unordered_set<Value*>;
using MoveSet = std::vector<Instruction*>;


class LoopInvHoist : public Pass {
public:
    LoopInvHoist(Module *m) : Pass(m) {}

    void run() override;
};
