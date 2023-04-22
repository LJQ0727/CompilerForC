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
};
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
        return 0;
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

    // std::vector<std::string> evaluate_expression();  // evaluation result saved in $t0
};

void codegen(ProductionRule rule, std::stack<Semantic> *semantic_stack);




