#include "semantic_routines.h"

using namespace std;

void codegen(ProductionRule rule, std::stack<Semantic> *semantic_stack) {
    // generate mips code upon reduction
    vector<Semantic> semantic_values;
    for (auto tok : rule.rhs) {
        semantic_values.insert(semantic_values.begin(), semantic_stack->top());
        semantic_stack->pop();
    }

    string res;

    if (rule.descriptor == "exp_int") {
        
    }
    else if (rule.descriptor == "exp_id") {

    }



    semantic_stack->push(res);
}