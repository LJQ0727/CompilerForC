#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <stack>
#include <map>

#include "parser.h"

map<string, int> symbol_table;  // map from variable name to memory location

// This enum is used to determine the type of a primary struct and guide to the corresponding data field
enum semantic_type {
    id, // id value is stored in a memory location
    literal,
    expression,

    terminal,   // the raw info directly from scanner
};

class Semantic {
public:
    Semantic(std::string terminal_value) {
        type = terminal;
        raw_value = terminal_value;
    }
    Semantic() {

    }
    // if is literal, then retrieve value from it
    // if is expression, then retrieve from the memory location {mem_location}($sp)
    // if is variable, then retrieve value from looking up symbol table
    enum semantic_type type;
    std::string variable_name;    // only variable has this
    int value;  // only int literal has this

    std::string raw_value;

    int mem_location;  // used to store expression

    std::vector<std::string> instructions;

    std::vector<std::string> evalute_expression();  // fill in the instructions, evaluation result saved in $t0
};

void codegen(ProductionRule rule, std::stack<Semantic> *semantic_stack);

// A linked list of symbols
// The symbols can be stored in register or a memory location. 
// Look up by traversing the list and matching the variable_name
struct symbol_table
{
    char variable_name[33]; // maximum 32-character variable name, with a trailing '\0'
    int reg_no;
    bool has_reg_no;
    int mem_location;
    bool has_mem_location;
    struct symbol_table* next;
};




// Create new primary struct from variable name
void new_primary_struct_id(Semantic* target, char* id);
// Create new primary struct from INTLITERAL
void new_primary_struct_literal(Semantic* target, int literal);

// inverse the value of the primary and store as an expression
void negative_primary_to_expression(Semantic* target, Semantic primary);

// expression PLUSOP primary
void plus_op(Semantic* target, Semantic expr1, Semantic primary);
void minus_op(Semantic* target, Semantic expr1, Semantic primary);

// A linked list of expressions
// used by write()
struct expr_list_struct
{
    Semantic expr; // The expression type shares the same struct with primary
    struct expr_list_struct* next;
};

// link the second expression after the first chain, forming an expression list
void concatenate_expr(Semantic exp1, Semantic exp2, struct expr_list_struct* target);
void concatenate_exprlist(struct expr_list_struct exp1, Semantic exp2, struct expr_list_struct* target);

// A linked list of variables
// used by read()
struct id_list_struct
{
    char id[33];    // variable name field
    struct id_list_struct* next;
};
// companion of idlist struct
void create_idlist(char* id1, char* id2, struct id_list_struct* target);
void append_idlist(struct id_list_struct* target, char* id);

// Traverse the symbol table and find a match
// If no match, create a new one initialized with 0
struct symbol_table* find_symbol(char* variable_name);

// Generate MIPS code for assign op
void assign_op(char* id, Semantic* expr);

// Generate MIPS code (syscalls) for IO of Micro language
void read_id(char* id);
void read_idlist(struct id_list_struct* idlist);
void write_expr(Semantic* expr);
void write_exprlist(struct expr_list_struct* exprlist);