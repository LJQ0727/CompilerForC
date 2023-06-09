/*
    File: semantic_routines.cpp
    Author: Jiaqi Li
    The semantic routines api for the Simplified C compiler, used by the parser.

    The `semantic_type` is used to determine the type of `Semantic` object and guide to the corresponding data field.
    The `SymbolTable` is used to store the symbol table for the compiler with scoping information.
    The `Semantic` is used to store the semantic information for each scanned token.

    `codegen` is the main function for code generation, which is called by the parser whenever a production rule is reduced.
*/

#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <stack>
#include <map>

#include "parser.h"

// This enum is used to determine the type of a primary struct and guide to the corresponding data field
enum semantic_type {
    id, // id value is stored in a memory location
    literal,
    expression,

    terminal,   // the raw info directly from scanner

    stmt,
};

extern int next_mem_location;

class SymbolTable {
public:
    SymbolTable() {
        // initialize the global scope
        tables.push_back(std::map<std::string, int>());
    }
    std::vector<std::map<std::string, int>> tables;
    int operator[](std::string key) {
        for (int i = tables.size() - 1; i >= 0; i--) {
            if (tables[i].find(key) != tables[i].end()) {
                return tables[i][key];
            }
        }
        add_symbol(key, next_mem_location);
        next_mem_location -= 4;
        return operator[](key);
    }
    void add_scope() {
        tables.push_back(std::map<std::string, int>());
    }
    void close_scope() {
        tables.pop_back();
    }
    void add_symbol(std::string key, int loc) {
        tables[tables.size() - 1][key] = loc;
    }
};
class Semantic {
public:
    Semantic(std::string terminal_value) {
        type = terminal;
        raw_value = terminal_value;
    }
    Semantic() {
        type = terminal;
        raw_value = "";
        value = 0;
        mem_location = 0;
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

    void push_back_instruction(std::string instruction) {
        instructions.push_back(std::string("\t") + instruction);
    }

    // return the label number
    int push_back_label();

    void printout() {
        for (std::string instruction : instructions) {
            std::cout << instruction << std::endl;
        }
        instructions.clear();
    }

    void merge_with(Semantic other) {
        instructions.insert(instructions.end(), other.instructions.begin(), other.instructions.end());
    }

    // std::vector<std::string> evaluate_expression();  // evaluation result saved in $t0
};

void codegen(ProductionRule rule, std::stack<Semantic> *semantic_stack);




