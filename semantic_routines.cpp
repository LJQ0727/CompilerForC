#include "semantic_routines.h"

using namespace std;


SymbolTable symbol_table;  // initialize the symbol table
int next_mem_location = -4; // start from -4, because $sp is pointing to the top of the stack

// std::vector<std::string> Semantic::evaluate_expression() {

// }

static void get_semantic_value(Semantic semantic, int reg_no, Semantic *new_semantic) {
    // store the value of semantics in $t{reg_no}
    switch (semantic.type)
    {
    case literal:
        new_semantic->instructions.push_back("li $t" + to_string(reg_no) + ", " + to_string(semantic.value));
        break;
    case expression:
        new_semantic->instructions.push_back("lw $t" + to_string(reg_no) + ", " + to_string(semantic.mem_location) + "($sp)");
        break;
    case id:
        new_semantic->instructions.push_back("lw $t" + to_string(reg_no) + ", " + to_string(symbol_table[semantic.variable_name]) + "($sp)");
        break;
    default:
        break;
    }
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
    else if (rule.descriptor == "id_decl_array") {
        // create symbol table entries for the array
        for (int i = 0; i < stoi(semantic_values[2].raw_value); i++) {
            symbol_table.add_symbol(semantic_values[0].raw_value + "[" + to_string(i) + "]", next_mem_location);
            next_mem_location -= 4;
        }
        new_semantic.type = id;
        new_semantic.variable_name = semantic_values[0].raw_value;
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

    // for every expression type, we need to store the result in a memory location
    else if (rule.descriptor == "id_idx") {
        // index the array
        new_semantic.type = expression;
        string key = semantic_values[0].raw_value + "[" + semantic_values[2].raw_value + "]";
        new_semantic.mem_location = symbol_table[key];
    }
    else if (rule.descriptor == "not_exp") {
        new_semantic = semantic_values[1];
        switch (new_semantic.type)
        {
        case literal:
            new_semantic.value = !new_semantic.value;
            break;
        case expression:
            new_semantic.instructions.push_back("lw $t0, " + to_string(new_semantic.mem_location) + "($sp)");
            new_semantic.instructions.push_back("sub $t0, $zero, $t0");
            new_semantic.mem_location = next_mem_location;
            next_mem_location -= 4;
            new_semantic.instructions.push_back("sw $t0, " + to_string(new_semantic.mem_location) + "($sp)");
            break;
        case id:
            new_semantic.type = expression;
            new_semantic.instructions.push_back("lw $t0, " + to_string(symbol_table[new_semantic.variable_name]) + "($sp)");
            new_semantic.instructions.push_back("sub $t0, $zero, $t0");
            new_semantic.mem_location = next_mem_location;
            next_mem_location -= 4;
            new_semantic.instructions.push_back("sw $t0, " + to_string(new_semantic.mem_location) + "($sp)");
            break;
        default:
            break;
        }
    }
    else if (rule.descriptor == "plus") {
        new_semantic.type = expression;
        get_semantic_value(semantic_values[0], 1, &new_semantic);
        get_semantic_value(semantic_values[2], 2, &new_semantic);
        new_semantic.instructions.push_back("add $t0, $t1, $t2");
        new_semantic.mem_location = next_mem_location;
        next_mem_location -= 4;
        new_semantic.instructions.push_back("sw $t0, " + to_string(new_semantic.mem_location) + "($sp)");
    }
    else if (rule.descriptor == "minus") {
        new_semantic.type = expression;
        get_semantic_value(semantic_values[0], 1, &new_semantic);
        get_semantic_value(semantic_values[2], 2, &new_semantic);
        new_semantic.instructions.push_back("sub $t0, $t1, $t2");
        new_semantic.mem_location = next_mem_location;
        next_mem_location -= 4;
        new_semantic.instructions.push_back("sw $t0, " + to_string(new_semantic.mem_location) + "($sp)");
    }
    else if (rule.descriptor == "mul") {
        new_semantic.type = expression;
        get_semantic_value(semantic_values[0], 1, &new_semantic);
        get_semantic_value(semantic_values[2], 2, &new_semantic);
        new_semantic.instructions.push_back("mul $t0, $t1, $t2");
        new_semantic.mem_location = next_mem_location;
        next_mem_location -= 4;
        new_semantic.instructions.push_back("sw $t0, " + to_string(new_semantic.mem_location) + "($sp)");
    }
    else if (rule.descriptor == "div") {
        new_semantic.type = expression;
        get_semantic_value(semantic_values[0], 1, &new_semantic);
        get_semantic_value(semantic_values[2], 2, &new_semantic);
        new_semantic.instructions.push_back("div $t1, $t2");
        new_semantic.instructions.push_back("mflo $t0");
        new_semantic.mem_location = next_mem_location;
        next_mem_location -= 4;
        new_semantic.instructions.push_back("sw $t0, " + to_string(new_semantic.mem_location) + "($sp)");
    }
    else if (rule.descriptor == "shl") {
        new_semantic.type = expression;
        get_semantic_value(semantic_values[0], 1, &new_semantic);
        get_semantic_value(semantic_values[2], 2, &new_semantic);
        new_semantic.instructions.push_back("sllv $t0, $t1, $t2");
        new_semantic.mem_location = next_mem_location;
        next_mem_location -= 4;
        new_semantic.instructions.push_back("sw $t0, " + to_string(new_semantic.mem_location) + "($sp)");
    }
    else if (rule.descriptor == "shr") {
        new_semantic.type = expression;
        get_semantic_value(semantic_values[0], 1, &new_semantic);
        get_semantic_value(semantic_values[2], 2, &new_semantic);
        new_semantic.instructions.push_back("srlv $t0, $t1, $t2");
        new_semantic.mem_location = next_mem_location;
        next_mem_location -= 4;
        new_semantic.instructions.push_back("sw $t0, " + to_string(new_semantic.mem_location) + "($sp)");
    }




    
    else if (rule.descriptor == "exp_id") {
        new_semantic.type = id;
        new_semantic.variable_name = semantic_values[0].raw_value;
    }
    else if (rule.descriptor == "exp_id") {
        new_semantic.type = id;
        new_semantic.variable_name = semantic_values[0].raw_value;
    }
    else if (rule.descriptor == "exp_id") {
        new_semantic.type = id;
        new_semantic.variable_name = semantic_values[0].raw_value;
    }
    else if (rule.descriptor == "exp_id") {
        new_semantic.type = id;
        new_semantic.variable_name = semantic_values[0].raw_value;
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