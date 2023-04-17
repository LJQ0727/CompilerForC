/*
    File: scanner.cpp
    Author: Jiaqi Li
    The scanner part for the C-like language compiler

    The scanner is implemented using a NFA and a DFA.
    The NFA is constructed according to the lecture slides, given some regular expression and the corresponding token.
    Then, the NFA is converted to a DFA using the subset construction algorithm.
    After that, the DFA is used to match the input code.
*/

#include <iostream>
#include <vector>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fstream>  // for reading file input
#include <sstream>  // for loading file content into string

#include <set>
#include <algorithm>    // for subset algorithm
#include "scanner.h"

using namespace std;
// declaration of scanner tokens
enum scanner_token {
    NUL_TOKEN = 0,

    // keywords
    INT, MAIN, VOID, BREAK, 
    DO, ELSE, IF, WHILE,
    RETURN, READ, WRITE,

    // special symbols
    LBRACE, RBRACE, LSQUARE, RSQUARE, 
    LPAR, RPAR, SEMI, PLUS,
    MINUS, MUL_OP, DIV_OP, AND_OP,
    OR_OP, NOT_OP, ASSIGN, LT,
    GT, SHL_OP, SHR_OP, EQ,
    NOTEQ, LTEQ, GTEQ, ANDAND,
    OROR, COMMA,

    // INT_NUM and ID
    INT_NUM, ID
};

std::vector<std::string> idx_to_token;
// State transitions for each state in NFA or DFA
class Transitions {
public:
    // collection of transitions
    // transition to another state number
    // the first is either a char or -1
    // if the key is -1, indicating a lambda (equivalent states)
    vector<pair<char, int>> transitions;
    
    // add a transition
    void add_transition(pair<char, int> tran);
};

class NFA;
class DFA;

// A class representing a state in NFA and DFA
class ScannerState {
public:
    // a Transitions object representing the transitions from this state
    // transition to another state number
    // the first is either a char or -1
    // if the key is -1, indicating a lambda (equivalent states)
    Transitions transitions;

    // only set in DFA, to indicate which NFA states this DFA state corresponds to
    set<int> nfa_counterparts;

    // transit into the next state
    // only should be called in DFA
    int transit(char key);

    int state_number;   // correspond to idx in `states`

    // the nfa or dfa this state belongs to
    NFA* nfa;
    DFA* dfa = nullptr;

    // if it is final state, when reaching this state the program can output the token
    bool is_final = false;
    scanner_token final_state_token = NUL_TOKEN;

    // a function to get the epsilon closure of the state
    set<int> get_epsilon_closure();
};

// A class representing a Non-Deterministic Finite Automaton (NFA)
// the NFA consists of a set of states, and transitions between states according to the input character
class NFA {
public:
    NFA() {
        // add start state
        ScannerState state;
        state.state_number = 0;
        add_state(state);
    }
    int start_state = 0;

    std::vector<ScannerState> states;  // collection of NFA states

    // add a standard regex (only simple words, not + or * or brackets or |. no space in between) to the NFA
    void add_standard_regex(string regex, scanner_token return_token);
    // add a regex that can parse int literals to the NFA
    void add_int_num_regex(scanner_token return_token);
    // add a regex that can parse ID to the NFA
    void add_id_regex(scanner_token return_token);

private:
    // add a state to the NFA
    void add_state(ScannerState state);

    // attach a token action to a certain state
    void attach_final_state_token(int state_no, scanner_token return_token);

    // nfa construction algorithms
    // returns pair of (start, finish) state no
    pair<int, int> new_char_nfa(char ch);
    pair<int, int> concatenate(int a_start, int a_finish, int b_start, int b_finish);
    pair<int, int> or_connection(int a_start, int a_finish, int b_start, int b_finish);
    pair<int, int> star_symbol(int a_start, int a_finish);
};

// A class representing a Deterministic Finite Automaton (DFA)
// the DFA consists of a set of states, and transitions between states according to the input character
class DFA {
public:
    // collection of DFA states
    std::vector<ScannerState> dfa_states;

    // the starting state index. Typically 0
    int start_state;

    // convert NFA to DFA, using subset construction algorithm
    void create_DFA(NFA* nfa);

    // driver that matches the code to the DFA
    void match_code(istream* code_istream, ostream* token_ostream, ostream* semantic_ostream);

};

// Implementation part

void Transitions::add_transition(pair<char, int> tran) {
    if (find(transitions.begin(), transitions.end(), tran) == transitions.end()) {
        transitions.push_back(tran);
    }
}

// Transit to the next state according to the input character
// it looks ahead a labmda transition if there is one
// returns -1 if no transition is found
int ScannerState::transit(char key) {
    for (pair<char, int> tran : transitions.transitions) {
        if (tran.first == key) {
            return tran.second;
        } else if (tran.first == -1) {
            // lambda transition
            if (dfa == nullptr) {
               for (pair<char, int> tran2 : nfa->states[tran.second].transitions.transitions) {
                    if (tran2.first == key) {
                        return tran2.second;
                    }
                } 
            } else {
                for (pair<char, int> tran2 : dfa->dfa_states[tran.second].transitions.transitions) {
                    if (tran2.first == key) {
                        return tran2.second;
                    }
                }
            }
        }
    }
    return -1;
}

// get the epsilon closure of the state
// the closure is a set of state numbers
set<int> ScannerState::get_epsilon_closure() {
    set<int> closure;
    // add itself
    closure.insert(state_number);

    for (pair<char, int> tran : transitions.transitions) {
        if (tran.first == -1 && tran.second != -1) {
            closure.insert(tran.second);

            if (tran.second != state_number) {
                set<int> child_closure = nfa->states[tran.second].get_epsilon_closure();
                closure.insert(child_closure.begin(), child_closure.end());
            }
        }

    }

    return closure;
}

void NFA::add_state(ScannerState state) {
    state.nfa = this;
    states.push_back(state);
}

// define the regex for the token, and create the NFA for it
void NFA::add_standard_regex(string regex, scanner_token return_token) {
    // standard regex: only simple words, not + or * or brackets or |. no space in between
    int current_state = start_state;
    for (int i = 0; i < regex.length(); i++) {
        if (states[current_state].transit(regex[i]) != -1) {
            // if the state already has a transition to the next state
            current_state = states[current_state].transit(regex[i]);
            if (i == regex.length()-1) {
                states[current_state].is_final = true;
                states[current_state].final_state_token = return_token;
            }
            continue;
        }

        pair<int, int> new_char = new_char_nfa(regex[i]);
        // connect the previous state to the new state
        pair<char, int> tran;
        tran.first = -1;
        tran.second = new_char.first;
        states[current_state].transitions.add_transition(tran);
        current_state = new_char.second;

        if (i == regex.length()-1) {
            attach_final_state_token(new_char.second, return_token);
        }

    }
}

// create a new NFA for integer literal
void NFA::add_int_num_regex(scanner_token return_token) {
    // int_num regex: [0-9]+
    // create [0-9]
    pair<int, int> char_nfa = new_char_nfa('0');
    for (int i = 1; i < 10; i++) {
        // add transition
        pair<char, int> tran;
        tran.first = '0'+i;
        tran.second = char_nfa.second;
        states[char_nfa.first].transitions.add_transition(tran);
    }
    // connect the start state to the first state with lambda
    pair<char, int> tran;
    tran.first = -1; // -1 means lambda (equivalent states)
    tran.second = char_nfa.first;
    states[start_state].transitions.add_transition(tran);

    // create another [0-9] nfa
    pair<int, int> char_nfa2 = new_char_nfa('0');
    for (int i = 1; i < 10; i++) {
        // add transition
        pair<char, int> tran;
        tran.first = '0'+i;
        tran.second = char_nfa2.second;
        states[char_nfa2.first].transitions.add_transition(tran);
    }
    // add * operator
    pair<int, int> star_nfa = star_symbol(char_nfa2.first, char_nfa2.second);
    // connect the first state to the star nfa with lambda
    pair<char, int> tran2;
    tran2.first = -1; // -1 means lambda (equivalent states)
    tran2.second = star_nfa.first;
    states[char_nfa.second].transitions.add_transition(tran2);

    // attach final state token
    attach_final_state_token(star_nfa.second, return_token);
}

// a helper function to disambiguate the id regex with the keyword regex
static void add_id_ending_transition(NFA* nfa, int start_state, pair<int, int> id_ending_star) {

    if (nfa->states[start_state].is_final) {
        // first create [0-9 | a-z A-Z | _] transition
        // and connect to the id_ending_star
        for (int i = 0; i < 10; i++) {
            // add 0-9 transition
            pair<char, int> tran;
            tran.first = '0'+i;
            tran.second = id_ending_star.first;
            nfa->states[start_state].transitions.add_transition(tran);
        }
        for (int i = 0; i < 26; i++) {
            // add a-z transition
            pair<char, int> tran;
            tran.first = 'a'+i;
            tran.second = id_ending_star.first;
            nfa->states[start_state].transitions.add_transition(tran);
        }
        for (int i = 0; i < 26; i++) {
            // add A-Z transition
            pair<char, int> tran;
            tran.first = 'A'+i;
            tran.second = id_ending_star.first;
            nfa->states[start_state].transitions.add_transition(tran);
        }
        // add '_' transition
        pair<char, int> tran;
        tran.first = '_';
        tran.second = id_ending_star.first;
        nfa->states[start_state].transitions.add_transition(tran);
        return;
    } else {
        nfa->states[start_state].final_state_token = ID;
    }

    // connect the nfa state to the id_ending_star
    // connect 0-9 A-Z, which have no ambiguity, to the id ending star
    for (int i = 0; i < 10; i++) {
        // add 0-9 transition
        pair<char, int> tran;
        tran.first = '0'+i;
        tran.second = id_ending_star.first;
        nfa->states[start_state].transitions.add_transition(tran);
    }
    for (int i = 0; i < 26; i++) {
        // add A-Z transition
        pair<char, int> tran;
        tran.first = 'A'+i;
        tran.second = id_ending_star.first;
        nfa->states[start_state].transitions.add_transition(tran);
    }
    // add '_' transition
    pair<char, int> tran;
    tran.first = '_';
    tran.second = id_ending_star.first;
    nfa->states[start_state].transitions.add_transition(tran);
    
    // deal with a-z ambiguity
    // ending star: [ 0-9 | a-z A-Z | _]∗
    for (int i = 0; i < 26; i++) {
        char first_letter = 'a'+i;
        if (nfa->states[start_state].transit(first_letter) == -1 || nfa->states[start_state].is_final) {
            // no current keywords starting with this letter, directly appending the nfa to the head
            nfa->states[start_state].transitions.add_transition({first_letter, id_ending_star.first});
        } else {
            int current_state = nfa->states[start_state].transit(first_letter);
            add_id_ending_transition(nfa, current_state, id_ending_star);
        }
    }
}

// add the id regex to the nfa, disambiguate the id regex with the keyword regex
void NFA::add_id_regex(scanner_token return_token) {
    // ID regex: [a−z A−Z][ 0-9 | a-z A-Z | _]∗

    // create [0-9 | a-z A-Z | _]*
    pair<int, int> id_ending = new_char_nfa('0');
    for (int i = 1; i < 10; i++) {
        // add 0-9 transition
        pair<char, int> tran;
        tran.first = '0'+i;
        tran.second = id_ending.second;
        states[id_ending.first].transitions.add_transition(tran);
    }
    for (int i = 0; i < 26; i++) {
        // add a-z transition
        pair<char, int> tran;
        tran.first = 'a'+i;
        tran.second = id_ending.second;
        states[id_ending.first].transitions.add_transition(tran);
    }
    for (int i = 0; i < 26; i++) {
        // add A-Z transition
        pair<char, int> tran;
        tran.first = 'A'+i;
        tran.second = id_ending.second;
        states[id_ending.first].transitions.add_transition(tran);
    }
    // add '_' transition
    pair<char, int> tran;
    tran.first = '_';
    tran.second = id_ending.second;
    states[id_ending.first].transitions.add_transition(tran);
    // add * operator
    pair<int, int> id_ending_star = star_symbol(id_ending.first, id_ending.second);
    // attach token to the finish state
    states[id_ending_star.second].final_state_token = return_token;


    for (int i = 0; i < 26; i++) {
        char first_letter = 'A'+i;
        if (states[start_state].transit(first_letter) == -1) {
            // no current keywords starting with this letter, directly add the nfa to the head
            states[start_state].transitions.add_transition({first_letter, id_ending_star.first});
        } else {
            assert(false);// currently there is not keyword with lowercase initials.
        }
    }
    for (int i = 0; i < 26; i++) {
        char first_letter = 'a'+i;
        if (states[start_state].transit(first_letter) == -1) {
            // no current keywords starting with this letter, directly add the nfa to the head
            states[start_state].transitions.add_transition({first_letter, id_ending_star.first});
        } else {
            int current_state = states[start_state].transit(first_letter);
            add_id_ending_transition(this, current_state, id_ending_star);
        }
    }

}

void NFA::attach_final_state_token(int state_no, scanner_token return_token) {
    this->states[state_no].final_state_token = return_token;
    this->states[state_no].is_final = true;
}

// Implementation of NFA construction and connection algorithms
// including concatenation, union, star, and character

pair<int, int> NFA::new_char_nfa(char ch) {
    ScannerState state1;
    ScannerState state2;
    state1.state_number = states.size();

    state2.state_number = state1.state_number + 1;

    pair<char, int> tran;
    tran.first = ch;
    tran.second = state2.state_number;
    state1.transitions.add_transition(tran);
    add_state(state1);
    add_state(state2);

    return pair<int, int>(state1.state_number, state2.state_number);
}

pair<int, int> NFA::concatenate(int a_start, int a_finish, int b_start, int b_finish) {
    pair<char, int> tran;
    tran.first = -1; // -1 means lambda (equivalent states)
    tran.second = b_start;
    states[a_finish].transitions.add_transition(tran);

    return pair<int, int>(a_start, b_finish);
}

pair<int, int> NFA::or_connection(int a_start, int a_finish, int b_start, int b_finish) {
    // create new start and finish state
    ScannerState state1;
    ScannerState state2;
    state1.state_number = states.size();

    state2.state_number = states.size()+1;

    pair<char, int> tran;
    tran.first = -1;
    tran.second = a_start;
    state1.transitions.add_transition(tran);

    tran.first = -1;
    tran.second = b_start;
    state1.transitions.add_transition(tran);

    tran.first = -1;
    tran.second = state2.state_number;
    states[a_finish].transitions.add_transition(tran);

    tran.first = -1;
    tran.second = state2.state_number;
    states[b_finish].transitions.add_transition(tran);

    add_state(state1);
    add_state(state2);
    return pair<int, int>(state1.state_number, state2.state_number);
}

pair<int, int> NFA::star_symbol(int a_start, int a_finish) {
    // create new start and finish state
    ScannerState start_state;
    ScannerState end_state;

    start_state.state_number = states.size();
    end_state.state_number = states.size()+1;

    // connect start state to end state with lambda
    pair<char, int> tran;
    tran.first = -1;
    tran.second = end_state.state_number;
    start_state.transitions.add_transition(tran);

    // connect end state to a_start with lambda
    tran.first = -1;
    tran.second = a_start;
    end_state.transitions.add_transition(tran);

    // connect a_finish to end state with lambda
    tran.first = -1;
    tran.second = end_state.state_number;
    states[a_finish].transitions.add_transition(tran);

    add_state(start_state);
    add_state(end_state);
    return pair<int, int>(start_state.state_number, end_state.state_number);
}

// Implementation of DFA construction algorithm with epsilon closure
// and subset construction
void DFA::create_DFA(NFA* nfa) {
    // convert NFA to DFA
    // get the epsilon closure of the start state
    set<int> start_state_closure = nfa->states[nfa->start_state].get_epsilon_closure();

    // the closure of each NFA state
    vector<set<int>> closures;

    // get the closure of each NFA state
    for (auto state : nfa->states) {
        set<int> closure = state.get_epsilon_closure();
        closures.push_back(closure);
    }

    // record the subsets to remove
    set<set<int>> subsets_to_remove;
    for (int i = 0; i < closures.size(); i++) {
        for (int j = 0; j < closures.size(); j++) {
            if (i != j) {
                if (includes(closures[i].begin(), closures[i].end(), closures[j].begin(), closures[j].end())) {
                    subsets_to_remove.insert(closures[j]);
                }
            }
        }
    }
    // add non-subset closures to the DFA states
    for (int i = 0; i < closures.size(); i++) {
        if (subsets_to_remove.find(closures[i]) == subsets_to_remove.end()) {
            ScannerState new_state;
            new_state.state_number = dfa_states.size();
            new_state.dfa = this;
            new_state.nfa = nfa;
            new_state.nfa_counterparts = closures[i];
            if (nfa->states[i].is_final) {
                new_state.is_final = true;
                new_state.final_state_token = nfa->states[i].final_state_token;
            } else {
                for (int nfa_counterpart : closures[i]) {
                    if (nfa->states[nfa_counterpart].final_state_token != -1 && nfa->states[nfa_counterpart].final_state_token != NUL_TOKEN) {
                        // new_state.is_final = true;
                        new_state.final_state_token = nfa->states[nfa_counterpart].final_state_token;
                        break;
                    }
                }
            }
            // add the new state to the DFA
            dfa_states.push_back(new_state);
        }
    }
    // create transitions between the DFA states
    for (int i = 0; i < dfa_states.size(); i++) {
        // for each of the DFA states, get the NFA states that it corresponds to
        set<int> nfa_counterparts = dfa_states[i].nfa_counterparts;
        // for each of the NFA states, get the transitions
        for (int nfa_counterpart : nfa_counterparts) {
            for (pair<char, int> tran : nfa->states[nfa_counterpart].transitions.transitions) {
                set<int> transition_closure = closures[tran.second];
                // match the transition closure to a DFA state
                for (int j = 0; j < dfa_states.size(); j++) {
                    if (i == j) continue;
                    // check where the transition closure is a subset of the DFA's NFA counterparts
                    // if it is, then add a transition from the DFA state to the DFA state
                    if (includes(
                        dfa_states[j].nfa_counterparts.begin(), 
                        dfa_states[j].nfa_counterparts.end(),
                        transition_closure.begin(),
                        transition_closure.end()
                    )) {
                        // insert such a transition
                        tran.second = j;
                        dfa_states[i].transitions.add_transition(tran);
                        continue;
                    }
                }
            }
        }
    }
    
    // determine which is the start state
    // start state is typically the first state in the NFA
    for (int i = 0; i < closures.size(); i++) {
        if (includes(closures[i].begin(), closures[i].end(), start_state_closure.begin(), start_state_closure.end())) {
            this->start_state = i;
            break;
        }
    }

}

// the driver function to match the code to the DFA given the code stream
void DFA::match_code(istream* code_istream, ostream* token_ostream, ostream* semantic_ostream)
{
    // match the code to the DFA
    int current_state = start_state;
    string semantic_value;  // the content of the token
    while (true) {
        char ch = code_istream->get();
        if (code_istream->eof()) {
            // when seeing EOF, check if the current state is a final state
            if (current_state != start_state && dfa_states[current_state].final_state_token != NUL_TOKEN) {
                *token_ostream << dfa_states[current_state].final_state_token << endl;
                *semantic_ostream << semantic_value << endl;
            }
            break;
        }

        // if seeing a whitespace, then check if the current state is a final state
        if (ch == ' ' || ch == '\n' || ch == '\r' || ch == '\0' || ch == '\t') {
            if (current_state != start_state && dfa_states[current_state].final_state_token != NUL_TOKEN) {
                *token_ostream << dfa_states[current_state].final_state_token << endl;
                *semantic_ostream << semantic_value << endl;
                current_state = start_state;
                semantic_value = "";
                continue;
            } else {
                continue;
            }

        }

        int next_state = dfa_states[current_state].transit(ch);
        // if the next character is not a valid transition, then check if the current state is a final state
        if (next_state != -1) {
            current_state = next_state;
            semantic_value += ch;
        }
        else {
            *token_ostream << dfa_states[current_state].final_state_token << endl;
            *semantic_ostream << semantic_value << endl;
            current_state = start_state;
            semantic_value = "";
            code_istream->unget();
        }
    }
}

// wraps the whole scanner, output token to an ostream
void scanner_driver(string input_fname, std::ostream* token_ostream, std::vector<std::string>* idx_to_token_copy, std::ostream* semantic_ostream)
{
    std::ifstream code_ifstream(input_fname);

    // encode the token name's corresponding index
    idx_to_token = {"NUL_TOKEN", "INT", "MAIN", "VOID", "BREAK", "DO", "ELSE", "IF", "WHILE", "RETURN", "READ", "WRITE", "LBRACE", "RBRACE", "LSQUARE", "RSQUARE", "LPAR", "RPAR", "SEMI", "PLUS", "MINUS", "MUL_OP", "DIV_OP", "AND_OP", "OR_OP", "NOT_OP", "ASSIGN", "LT", "GT", "SHL_OP", "SHR_OP", "EQ", "NOTEQ", "LTEQ", "GTEQ", "ANDAND", "OROR", "COMMA", "INT_NUM", "ID"};
    *idx_to_token_copy = idx_to_token;

    // encode the regex into nfa
    NFA nfa;

    // encode INT_NUM and ID
    nfa.add_int_num_regex(INT_NUM);

    // encode keywords
    nfa.add_standard_regex("int", INT);
    nfa.add_standard_regex("main", MAIN);
    nfa.add_standard_regex("void", VOID);
    nfa.add_standard_regex("break", BREAK);
    nfa.add_standard_regex("do", DO);
    nfa.add_standard_regex("else", ELSE);
    nfa.add_standard_regex("if", IF);
    nfa.add_standard_regex("while", WHILE);
    nfa.add_standard_regex("return", RETURN);
    nfa.add_standard_regex("scanf", READ);
    nfa.add_standard_regex("printf", WRITE);

    // encode operators
    nfa.add_standard_regex("{", LBRACE);
    nfa.add_standard_regex("}", RBRACE);
    nfa.add_standard_regex("[", LSQUARE);
    nfa.add_standard_regex("]", RSQUARE);
    nfa.add_standard_regex("(", LPAR);
    nfa.add_standard_regex(")", RPAR);
    nfa.add_standard_regex(";", SEMI);
    nfa.add_standard_regex("+", PLUS);
    nfa.add_standard_regex("-", MINUS);
    nfa.add_standard_regex("*", MUL_OP);
    nfa.add_standard_regex("/", DIV_OP);
    nfa.add_standard_regex("&", AND_OP);
    nfa.add_standard_regex("|", OR_OP);
    nfa.add_standard_regex("!", NOT_OP);
    nfa.add_standard_regex("=", ASSIGN);
    nfa.add_standard_regex("<", LT);
    nfa.add_standard_regex(">", GT);
    nfa.add_standard_regex("<<", SHL_OP);
    nfa.add_standard_regex(">>", SHR_OP);
    nfa.add_standard_regex("==", EQ);
    nfa.add_standard_regex("!=", NOTEQ);
    nfa.add_standard_regex("<=", LTEQ);
    nfa.add_standard_regex(">=", GTEQ);
    nfa.add_standard_regex("&&", ANDAND);
    nfa.add_standard_regex("||", OROR);
    nfa.add_standard_regex(",", COMMA);

    nfa.add_id_regex(ID);

    // create the DFA from the NFA
    // by converting the NFA to a DFA
    DFA dfa;
    dfa.create_DFA(&nfa);

    // match code to the DFA
    dfa.match_code(&code_ifstream, token_ostream, semantic_ostream);
    code_ifstream.close();
}
