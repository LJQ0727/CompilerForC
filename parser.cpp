/* File: parser.cpp
    Author: Jiaqi Li
    Implementation of the parser for a subset of C language
    using LR(1) parsing method and supports context-free grammars
*/

#include <stack>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <set>
#include <cassert>
#include <map>
#include <algorithm>

#include "scanner.h"
#include "parser.h"

using namespace std;

static std::vector<std::string> idx_to_token_copy; // transform index number to token strings

class ProductionRule;
class LROneParser;

// simulate stream behavior but with tokens
class TokenStream {
public:
    vector<parser_token> tokens;
    int idx = 0;
    void unget() {
        if (idx > 0) {
            idx -= 1;
        }
    }
    parser_token get() {
        if (idx < tokens.size()) {
            return tokens[idx++];
        } else {
            return SCANEOF;
        }
    }
    void push_back(parser_token tok) {
        tokens.push_back(tok);
    }
};

// a production rule with a 'dot',
// representing the parsing status
class ProductionRule {
public:
    parser_token lhs;  // left-hand side token
    vector<parser_token> rhs;  // right-hand side tokens; in their order

    int dot_location;   // the dot is placed before the index
    set<parser_token> lookaheads;    // the look-ahead part in LR(1)

    ProductionRule advance_dot();

    int index;

    bool is_end() {
        return dot_location >= rhs.size();
    }

    parser_token get_next_token() {
        assert(!is_end());
        return rhs[dot_location];
    }
};

// a state in the LR(1) parser
class ItemSet {
public:
    ItemSet() {
        for (int i = 0; i < 100; i++) {
            goto_table[i] = -1;
        }
    }
    set<ProductionRule> original_prod_rules;  // the original production rules in this state
    set<ProductionRule> all_prod_rules;   // the parsing status of the production rule in this state, including original and derived

    int state_number;
    LROneParser* parser;

    int goto_table[100];

    void build_closure();

    set<ProductionRule> get_rules_with_lhs(parser_token lhs);

    // get the first set of a nonterminal, looking only at all production rules
    set<parser_token> get_first_set(parser_token target);

    // get the follow set of a nonterminal, looking only at production rules in this state
    set<parser_token> get_follow_set(parser_token target, set<parser_token>& visited);
};

// the parser driver
class LROneParser {
public: 
    set<ProductionRule> prod_rules; // all production rules
    vector<ItemSet*> parser_states;

    int curr_state = 0; // current parsing state

    parser_token start_token = system_goal;

    map<parser_token, bool> derives_lambda;  // whether a nonterminal derives lambda

    // add a production rule
    void register_prod_rule(parser_token lhs, vector<parser_token> rhs);

    // returns the added or queryed state number
    pair<int, bool> add_or_query_state(set<ProductionRule> target);

    void construct_parser(parser_token start_rule_lhs, vector<parser_token> start_rule_rhs);

    void parse(TokenStream* input_stream);

};
// used for building binary tree in set data structure
bool operator<(const ProductionRule& lhs, const ProductionRule& rhs) {
    // Return true if lhs is strictly less than rhs, and false otherwise
    if (lhs.lhs != rhs.lhs) {
        return lhs.lhs < rhs.lhs;
    } else if (lhs.rhs != rhs.rhs) {
        return lhs.rhs < rhs.rhs;
    } else if (lhs.dot_location != rhs.dot_location) {
        return lhs.dot_location < rhs.dot_location;
    } else {
        return lhs.lookaheads < rhs.lookaheads;
    }
}
bool operator==(const ProductionRule& lhs, const ProductionRule& rhs) {
    return (lhs.lhs == rhs.lhs) && (lhs.rhs == rhs.rhs) && (lhs.dot_location == rhs.dot_location) && (lhs.lookaheads == rhs.lookaheads);
}

// determine if a token is an operator
static bool is_operator(parser_token tok) {
    return (tok == PLUS) || (tok == MINUS) || (tok == MUL_OP) || (tok == DIV_OP)
    || (tok == NOT_OP) || (tok == ANDAND) || (tok == OROR) || (tok == EQ)
    || (tok == NOTEQ) || (tok == OR_OP) || (tok == AND_OP) || (tok == SHL_OP)
    || (tok == SHR_OP) || (tok == LT) || (tok == LTEQ) || (tok == GTEQ) || (tok == GT);
}

// print "|" before index `pos`
static void print_token_stack(vector<parser_token> token_stack, int pos) {
    cout << "current situation: ";
    for (int i = 0; i < token_stack.size(); i++) {
        if (i == pos) {
            cout << "| ";
        }
        cout << idx_to_token_copy[token_stack.at(i)] << " ";
    }
    if (pos == token_stack.size()) {
        cout << "| ";
    }
    cout << "\n\n";
}

void LROneParser::parse(TokenStream* input_stream) {
    curr_state = 0;
    stack<int> state_stack;
    stack<parser_token> operator_stack;
    state_stack.push(0);
    parser_token next_token;
    vector<parser_token> token_stack;

    // encode operator precedence, using the value from cppreference.com
    map<parser_token, int> precedence_table;
    
    precedence_table[PLUS] = 11;
    precedence_table[MINUS] = 11;

    precedence_table[MUL_OP] = 12;
    precedence_table[DIV_OP] = 12;
    precedence_table[NOT_OP] = 14;

    precedence_table[ANDAND] = 3;
    precedence_table[OROR] = 2;

    precedence_table[EQ] = 7;
    precedence_table[NOTEQ] = 7;

    precedence_table[OR_OP] = 4;
    precedence_table[AND_OP] = 6;
    
    precedence_table[SHL_OP] = 10;
    precedence_table[SHR_OP] = 10;

    precedence_table[LT] = 8;
    precedence_table[LTEQ] = 8;
    precedence_table[GTEQ] = 8;
    precedence_table[GT] = 8;


    while (true) {
        next_token = input_stream->get();

        cout << "state: " << curr_state << "\t" << "next type: " << idx_to_token_copy[next_token] << "\t\t";

        bool can_shift = false;
        // check if can shift
        if (parser_states[curr_state]->goto_table[next_token] != -1) {
            can_shift = true;
        }
        bool can_reduce = false;
        parser_token reduced_token;
        // check if can be reduced
        for (auto rule : parser_states[curr_state]->all_prod_rules) {
            if (rule.is_end() && rule.lookaheads.count(next_token)) {
                can_reduce = true;
                reduced_token = rule.lhs;
                break;
            }
        }
        if (!can_shift && !can_reduce) {
            cout << "error" << endl;
            return;
        }
        if (can_shift && can_reduce) {
            // apply operator precedence
            if (operator_stack.empty()) {
                // delay reduce
                can_reduce = false;
            } else if (precedence_table[next_token] > precedence_table[operator_stack.top()]) {
                can_reduce = false;
            } else {
               // perform reduce first, which is the default
            }
        }

        if (can_reduce) {
            for (auto rule : parser_states[curr_state]->all_prod_rules) {
                if (rule.is_end() && rule.lookaheads.count(next_token)) {
                    // reduce
                    reduced_token = rule.lhs;
                    cout << "reduce by grammar " << rule.index+1 << ": " << idx_to_token_copy[rule.lhs] << "->";
                    if (rule.rhs.size() == 0) {
                        cout << "lambda" << endl;
                    } else {
                        for (auto tok : rule.rhs) {
                            cout << idx_to_token_copy[tok] << " ";
                            if (is_operator(tok)) {
                                operator_stack.pop();
                            }
                            token_stack.pop_back();
                        }
                        cout << endl;
                        token_stack.push_back(reduced_token);
                        print_token_stack(token_stack, token_stack.size()-1);
                        token_stack.pop_back();
                    }
                    for (int i = 0; i < rule.rhs.size(); i++) {
                        state_stack.pop();
                    }
                    curr_state = state_stack.top();
                    // state_stack.push(parser_states[curr_state]->goto_table[rule.lhs]);
                    // curr_state = state_stack.top();

                    input_stream->unget();
                    next_token = reduced_token;
                    cout << "state: " << curr_state << "\t" << "next type: " << idx_to_token_copy[next_token] << "\t\t";
                    break;
                }
            }
        }

        // check if can be shifted
        if (parser_states[curr_state]->goto_table[next_token] != -1) {
            // add shifted operator to stack
            if (is_operator(next_token)) {
                operator_stack.push(next_token);
            }
            // shift
            cout << "shift to state " << parser_states[curr_state]->goto_table[next_token] << endl;
            state_stack.push(parser_states[curr_state]->goto_table[next_token]);
            curr_state = state_stack.top();
            token_stack.push_back(next_token);
            print_token_stack(token_stack, token_stack.size());
            // accept when the whole code is reduced to program
            if (next_token == SCANEOF) {
                cout << "Accept!" << endl;
                return;
            }
        } else {
            cout << "error" << endl;
            return;
        }
    }
}

void LROneParser::register_prod_rule(parser_token lhs, vector<parser_token> rhs) {
    ProductionRule new_rule;
    new_rule.lhs = lhs;
    new_rule.rhs = rhs;
    new_rule.dot_location = 0;
    new_rule.index = prod_rules.size();
    // new_rule.parser = this;
    // new_rule.lookaheads = NOTHING;
    prod_rules.insert(new_rule);
}

// after adding production rules, construct the parser
void LROneParser::construct_parser(parser_token start_rule_lhs, vector<parser_token> start_rule_rhs) {
    // first, find out which nonterminals derive lambda
    set<parser_token> vocabulary;
    for (ProductionRule rule : prod_rules) {
        vocabulary.insert(rule.lhs);
        for (parser_token tok : rule.rhs) {
            vocabulary.insert(tok);
        }
    }
    for (parser_token tok : vocabulary) {
        derives_lambda[tok] = false;
    }
    bool change = true;
    while (change) {
        change = false;
        for (auto rule: prod_rules) {
            if (!derives_lambda[rule.lhs]) {
                if (rule.rhs.size() == 0) {
                    change = true;
                    derives_lambda[rule.lhs] = true;
                    continue;
                } else {
                    // does each part derive lambda?
                    bool all_derive_lambda = true;
                    for (parser_token tok : rule.rhs) {
                        if (!derives_lambda[tok]) {
                            all_derive_lambda = false;
                            break;
                        }
                    }
                    if (all_derive_lambda) {
                        change = true;
                        derives_lambda[rule.lhs] = true;
                    }
                }
            }
        }
    }

    // add the start rule in case it is not there
    bool start_rule_exists = false;
    for (auto rule : prod_rules) {
        if (rule.lhs == start_rule_lhs && rule.rhs == start_rule_rhs) {
            start_rule_exists = true;
            break;
        }
    }
    if (!start_rule_exists) register_prod_rule(start_rule_lhs, start_rule_rhs);

    // add the first state using the start rule
    ProductionRule start_rule;
    start_rule.lhs = start_rule_lhs;
    start_rule.rhs = start_rule_rhs;
    start_rule.dot_location = 0;
    start_rule.lookaheads = {SCANEOF};
    // start_rule.parser = this;

    int start_state_number = add_or_query_state({start_rule}).first;
    assert(start_state_number == 0);
    parser_states[start_state_number]->build_closure();

}

// get all the rules lhs matching a given lhs
set<ProductionRule> ItemSet::get_rules_with_lhs(parser_token lhs) {
    set<ProductionRule> ret;
    for (ProductionRule rule : parser->prod_rules) {
        if (rule.lhs == lhs) {
            rule.dot_location = 0;
            ret.insert(rule);
        }
    }
    return ret;
}

set<parser_token> ItemSet::get_first_set(parser_token target) {
    set<parser_token> ret;
    if (parser->derives_lambda[target]) {
        ret.insert(LAMBDA);
    }
    if (is_terminal_token(target)) {
        return {target};
    } else {
        for (auto rule : parser->prod_rules) {
            if (rule.lhs == target) {
                for (parser_token first_token : rule.rhs) {
                    if (first_token == target) {
                        continue;
                    }
                    set<parser_token> first_set = get_first_set(first_token);
                    bool has_lambda = first_set.find(LAMBDA) != first_set.end();
                    if (has_lambda) {
                        first_set.erase(LAMBDA);
                        ret.insert(first_set.begin(), first_set.end());
                    } else {
                        ret.insert(first_set.begin(), first_set.end());
                        break;
                    }
                }
            }
        }
    }

    for (auto tok : ret) {
        assert(is_terminal_token(tok));
    }
    return ret;
}

set<parser_token> ItemSet::get_follow_set(parser_token target, set<parser_token>& visited) {

    set<parser_token> ret;
    if (target == parser->start_token) {
        ret.insert(SCANEOF);
    }
    for (auto rule : all_prod_rules) {
        if (rule.lhs == target && (!rule.lookaheads.empty())) {
            for (parser_token lookahead : rule.lookaheads) {
                ret.insert(lookahead);
            }
            return ret;
        }
    }
    for (auto rule : all_prod_rules) {
        for (int i = 0; i < rule.rhs.size(); i++) {
            if (rule.rhs[i] == target) {
                if (i == rule.rhs.size() - 1) {
                    // last token in the rule
                    if (rule.lhs != target) {
                        if (visited.find(rule.lhs) == visited.end()) {
                            visited.insert(rule.lhs);
                            set<parser_token> follow_set = get_follow_set(rule.lhs, visited);
                            ret.insert(follow_set.begin(), follow_set.end());
                        }
                    } 
                } else {
                    // not last token in the rule
                    parser_token next_token = rule.rhs[i + 1];
                    set<parser_token> first_set = get_first_set(next_token);
                    bool has_lambda = first_set.find(LAMBDA) != first_set.end();
                    if (has_lambda) {
                        first_set.erase(LAMBDA);
                        ret.insert(first_set.begin(), first_set.end());
                        if (visited.find(next_token) == visited.end()) {
                            visited.insert(next_token);
                            set<parser_token> follow_set = get_follow_set(next_token, visited);
                            ret.insert(follow_set.begin(), follow_set.end());
                        }
                    } else {
                        ret.insert(first_set.begin(), first_set.end());
                    }
                }
            }
        }
    }
    return ret;
}

// compare two production rules, ignoring lookahead
bool compare_equal_no_lookahead(ProductionRule a, ProductionRule b) {
    return (a.lhs == b.lhs) && (a.rhs == b.rhs) && (a.dot_location == b.dot_location);
}

// build up the first itemset, and recursively build others
void ItemSet::build_closure() {
    // first, fill the item set but do not fill lookahead
    // repeatedly add new items to the closure until no new items can be added
    bool change = true;
    all_prod_rules = original_prod_rules;

    while (change) {
        change = false;
        for (ProductionRule rule : all_prod_rules) {
            if (rule.is_end()) {
                // do nothing
            } else {
                parser_token next_token = rule.get_next_token();
                if (is_terminal_token(next_token)) {
                    // no additional item to add
                } else {
                    // find all production rules with lhs the nonterminal
                    set<ProductionRule> other_rules = get_rules_with_lhs(next_token);
                    // if is new, add to closure
                    for (ProductionRule other_rule : other_rules) {
                        bool is_new = true;
                        for (ProductionRule tmp_rule : all_prod_rules) {
                            if (compare_equal_no_lookahead(tmp_rule, other_rule)) {
                                is_new = false;
                                break;
                            }
                        }
                        if (is_new) {
                            all_prod_rules.insert(other_rule);
                            change = true;
                        }
                    }
                }
            }
        }
    }

    set<ProductionRule> tmp_prod_rules;
    // determine the lookahead for each rule
    for (ProductionRule rule : all_prod_rules) {
        if (rule.lookaheads.empty()) {
            set<parser_token> visited;
            rule.lookaheads = get_follow_set(rule.lhs, visited);
        }
        tmp_prod_rules.insert(rule);
    }
    all_prod_rules = tmp_prod_rules;

    // create new item sets
    // group by next token
    set<parser_token> next_tokens;
    for (ProductionRule rule : all_prod_rules) {
        if (!rule.is_end()) {
            next_tokens.insert(rule.get_next_token());
        }
    }
    // for each next token, create new state or use existing state
    for (parser_token tok : next_tokens) {
        // filter out the supporting rules
        set<ProductionRule> new_rules;
        for (auto rule : all_prod_rules) {
            if (!rule.is_end()) {
                if (rule.get_next_token() == tok) {
                    new_rules.insert(rule.advance_dot());
                }
            }
        }
        // create new state with the rules
        pair<int, bool> newstate_info = parser->add_or_query_state(new_rules);
        int newstate_number = newstate_info.first;
        bool newstate_created = newstate_info.second;

        // if the new state is newly-created, build it up
        if (newstate_created) {
            parser->parser_states[newstate_number]->build_closure();
        }
        goto_table[(int)tok] = newstate_number;
    }
}

// return the state number of the new state or the existing state
// second value is true if the state is newly created
pair<int, bool> LROneParser::add_or_query_state(set<ProductionRule> target) {
    for (ItemSet* state : parser_states) {
        if (includes(state->all_prod_rules.begin(), state->all_prod_rules.end(),
                     target.begin(), target.end()) &&
            includes(target.begin(), target.end(),
                     state->original_prod_rules.begin(), state->original_prod_rules.end())) {
                return pair<int, bool>(state->state_number, false);
            }
    }
    // no existing state, add new state
    ItemSet* new_state = new ItemSet();
    new_state->state_number = parser_states.size();
    new_state->original_prod_rules = target;
    new_state->parser = this;
    parser_states.push_back(new_state);
    return pair<int, bool>(new_state->state_number, true);
}

// return the production rule with dot position advanced by 1
ProductionRule ProductionRule::advance_dot() {
    ProductionRule new_rule = *this;
    new_rule.dot_location += 1;
    return new_rule;
}

int main(int argc, char const *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Missing input file!\n");
    }
    stringstream ss;
    scanner_driver(string(argv[1]), &ss, &idx_to_token_copy);

    TokenStream tokens;    // store the scanned tokens, used by parser

    // encode the names for nonterminal tokens
    vector<string> nonterminal_tokens = {
        "program", 
        "var_declarations", "var_declaration",
        "declaration_list", "declaration", 
        "code_block",
        "statements", "statement",
        "control_statement", 
        "while_statement", "do_while_statement",
        "return_statement", 
        "read_write_statement",
        "read_statement", "write_statement",
        "assign_statement",
        "if_statement", "if_stmt", 
        "exp", "LAMBDA", "SCANEOF",
        "primary", "system_goal",
    };
    for (string token : nonterminal_tokens) {
        idx_to_token_copy.push_back(token);
    }

    // print out the scanned tokens
    cout << "Scanned Tokens: " << endl;
    while (true)
    {
        int a;
        ss >> a;
        if (ss.eof())
        {
            tokens.push_back(SCANEOF);
            cout << "SCANEOF\n\n";
            break;
        }
        
        cout << idx_to_token_copy[a] << " ";
        tokens.push_back((parser_token)a);
    }

    LROneParser parser;
    parser.register_prod_rule(program, vector<parser_token>{var_declarations, statements});
    parser.register_prod_rule(program, vector<parser_token>{statements});
    
    parser.register_prod_rule(var_declarations, vector<parser_token>{var_declaration});
    parser.register_prod_rule(var_declarations, vector<parser_token>{var_declarations, var_declaration});
    parser.register_prod_rule(var_declaration, vector<parser_token>{INT, declaration_list, SEMI});

    parser.register_prod_rule(declaration_list, vector<parser_token>{declaration});
    parser.register_prod_rule(declaration_list, vector<parser_token>{declaration_list, COMMA, declaration});
    parser.register_prod_rule(declaration, vector<parser_token>{ID});
    parser.register_prod_rule(declaration, vector<parser_token>{ID, ASSIGN, INT_NUM});
    parser.register_prod_rule(declaration, vector<parser_token>{ID, LSQUARE, INT_NUM, RSQUARE});

    parser.register_prod_rule(code_block, vector<parser_token>{statement});
    parser.register_prod_rule(code_block, vector<parser_token>{LBRACE, statements, RBRACE});

    parser.register_prod_rule(statements, vector<parser_token>{statement});
    parser.register_prod_rule(statements, vector<parser_token>{statements, statement});

    parser.register_prod_rule(statement, vector<parser_token>{assign_statement, SEMI});
    parser.register_prod_rule(statement, vector<parser_token>{control_statement});
    parser.register_prod_rule(statement, vector<parser_token>{read_write_statement, SEMI});
    parser.register_prod_rule(statement, vector<parser_token>{SEMI});

    parser.register_prod_rule(control_statement, vector<parser_token>{if_statement});
    parser.register_prod_rule(control_statement, vector<parser_token>{while_statement});
    parser.register_prod_rule(control_statement, vector<parser_token>{do_while_statement, SEMI});
    parser.register_prod_rule(control_statement, vector<parser_token>{return_statement, SEMI});

    parser.register_prod_rule(read_write_statement, vector<parser_token>{read_statement});
    parser.register_prod_rule(read_write_statement, vector<parser_token>{write_statement});

    parser.register_prod_rule(assign_statement, vector<parser_token>{ID, LSQUARE, exp, RSQUARE, ASSIGN, exp});
    parser.register_prod_rule(assign_statement, vector<parser_token>{ID, ASSIGN, exp});

    parser.register_prod_rule(if_statement, vector<parser_token>{if_stmt});
    parser.register_prod_rule(if_statement, vector<parser_token>{if_stmt, ELSE, code_block});

    // if stmt : IF LPAR exp RPAR code block
    // 14. while statement : WHILE LPAR exp RPAR code block
    // 15. do while statement: DO code block WHILE LPAR exp RPAR
    // 16. return statement : RETURN
    // 17. read statement: READ LPAR IDENTIFIER RPAR
    // 18. write statement : WRITE LPAR exp RPAR
    // 19. exp: INT NUM | IDENTIFIER | IDENTIFIER LSQUARE exp LSQUARE | NOT OP exp | exp AND OP exp |
    // exp OR OP exp | exp PLUS exp | exp MINUS exp | exp MUL OP exp | exp DIV OP exp |
    // exp LT exp | exp GT exp | exp EQ exp | exp NOTEQ exp | exp LTEQ exp | exp GTEQ exp |
    // exp SHL OP exp | exp SHR OP exp | exp ANDAND exp | exp OROR exp | LPAR exp RPAR |
    // MINUS exp

    parser.register_prod_rule(if_stmt, vector<parser_token>{IF, LPAR, exp, RPAR, code_block});
    parser.register_prod_rule(while_statement, vector<parser_token>{WHILE, LPAR, exp, RPAR, code_block});
    parser.register_prod_rule(do_while_statement, vector<parser_token>{DO, code_block, WHILE, LPAR, exp, RPAR});
    parser.register_prod_rule(return_statement, vector<parser_token>{RETURN});
    parser.register_prod_rule(read_statement, vector<parser_token>{READ, LPAR, ID, RPAR});
    parser.register_prod_rule(write_statement, vector<parser_token>{WRITE, LPAR, exp, RPAR});


    parser.register_prod_rule(exp, vector<parser_token>{INT_NUM});
    parser.register_prod_rule(exp, vector<parser_token>{ID});
    parser.register_prod_rule(exp, vector<parser_token>{ID, LSQUARE, exp, RSQUARE});
    parser.register_prod_rule(exp, vector<parser_token>{NOT_OP, exp});
    parser.register_prod_rule(exp, vector<parser_token>{exp, PLUS, exp});
    parser.register_prod_rule(exp, vector<parser_token>{exp, MINUS, exp});
    parser.register_prod_rule(exp, vector<parser_token>{exp, MUL_OP, exp});
    parser.register_prod_rule(exp, vector<parser_token>{exp, DIV_OP, exp});
    parser.register_prod_rule(exp, vector<parser_token>{exp, SHL_OP, exp});
    parser.register_prod_rule(exp, vector<parser_token>{exp, SHR_OP, exp});
    parser.register_prod_rule(exp, vector<parser_token>{exp, AND_OP, exp});
    parser.register_prod_rule(exp, vector<parser_token>{exp, OR_OP, exp});
    parser.register_prod_rule(exp, vector<parser_token>{exp, ANDAND, exp});
    parser.register_prod_rule(exp, vector<parser_token>{exp, OROR, exp});
    parser.register_prod_rule(exp, vector<parser_token>{exp, EQ, exp});
    parser.register_prod_rule(exp, vector<parser_token>{exp, NOTEQ, exp});
    parser.register_prod_rule(exp, vector<parser_token>{exp, LT, exp});
    parser.register_prod_rule(exp, vector<parser_token>{exp, GT, exp});
    parser.register_prod_rule(exp, vector<parser_token>{exp, LTEQ, exp});
    parser.register_prod_rule(exp, vector<parser_token>{exp, GTEQ, exp});

    parser.register_prod_rule(exp, vector<parser_token>{LPAR, exp, RPAR});

    parser.register_prod_rule(exp, vector<parser_token>{MINUS, exp});
    parser.register_prod_rule(exp, vector<parser_token>{PLUS, exp});


    parser.construct_parser(system_goal, vector<parser_token>{program, SCANEOF});
    cout << "Parsing Process: \n";
    parser.parse(&tokens);

    return 0;
}
