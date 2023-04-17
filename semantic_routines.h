#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <stack>
#include "scanner.h"
#include "parser.h"

void codegen(ProductionRule rule, std::stack<std::string> *semantic_stack) {
    // generate mips code upon reduction
    for (auto tok : rule.rhs) {
        semantic_stack->pop();
    }
    semantic_stack->push("temp");
}