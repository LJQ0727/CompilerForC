#include "semantic_routines.h"

using namespace std;


SymbolTable symbol_table;  // initialize the symbol table
int next_mem_location = -4; // start from -4, because $sp is pointing to the top of the stack

std::vector<std::string> Semantic::evaluate_expression() {

}



void codegen(ProductionRule rule, std::stack<Semantic> *semantic_stack) {
    // generate mips code upon reduction
    vector<Semantic> semantic_values;
    for (auto tok : rule.rhs) {
        semantic_values.insert(semantic_values.begin(), semantic_stack->top());
        semantic_stack->pop();
    }

    Semantic new_semantic;

    // declarations
    if (rule.descriptor == "id_decl") {
        // create a new symbol table entry
        symbol_table.add_symbol(semantic_values[0].raw_value, next_mem_location);
        next_mem_location -= 4;
        new_semantic.type = id;
        new_semantic.variable_name = semantic_values[0].raw_value;
    }
    else if (rule.descriptor == "id_decl_init") {
        // create a new symbol table entry
        symbol_table.add_symbol(semantic_values[0].raw_value, next_mem_location);
        next_mem_location -= 4;
        new_semantic.type = id;
        new_semantic.variable_name = semantic_values[0].raw_value;
        // evaluate the expression
        vector<string> instructions = semantic_values[2].evaluate_expression();
        // save the result to the memory location of id
        int mem_loc = symbol_table[semantic_values[0].raw_value];
        instructions.push_back("sw $t0, " + to_string(mem_loc) + "($sp)");
        new_semantic.instructions = instructions;
    }

    // dealing with expressions
    else if (rule.descriptor == "exp_int") {
        new_semantic.type = literal;
        new_semantic.value = stoi(semantic_values[0].raw_value);
    }
    else if (rule.descriptor == "exp_id") {
        new_semantic.type = id;
        new_semantic.variable_name = semantic_values[0].raw_value;
    }
    else if (rule.descriptor == "plusexp") {
        new_semantic = semantic_values[1];
    }
    else if (rule.descriptor == "parexp") {
        new_semantic = semantic_values[1];
    }
    else if (rule.descriptor == "id_init") {
        new_semantic.type = id;
        new_semantic.variable_name = semantic_values[0].raw_value;
        vector<string> instructions = semantic_values[2].evaluate_expression();
        // save the result to the memory location of id
        int mem_loc = symbol_table[new_semantic.variable_name];
        instructions.push_back("sw $t0, " + to_string(mem_loc) + "($sp)");
        new_semantic.instructions = instructions;
    }


    
    else if (rule.descriptor == "exp_id") {
        new_semantic.type = id;
        new_semantic.variable_name = semantic_values[0].raw_value;
    }
    else if (rule.descriptor == "exp_id") {
        new_semantic.type = id;
        new_semantic.variable_name = semantic_values[0].raw_value;
    }
    

    semantic_stack->push(new_semantic);
}