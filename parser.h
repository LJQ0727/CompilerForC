// declaration of scanner tokens
// the non-terminals must follow the same defined in "scanner.cpp"
enum parser_token {
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
    INT_NUM, ID,

    // Nonterminals -----------
    // program
    program, 
    // declarations
    var_declarations, var_declaration,
    declaration_list, declaration, 
    // codeblock
    code_block,
    // statements
    statements, statement,
    control_statement, 
    while_statement, do_while_statement,
    return_statement, 
    read_write_statement,
    read_statement, write_statement,
    assign_statement,
    if_statement, if_stmt,  // if-else case
    // expression
    exp,

    LAMBDA,     // will not explicitly write 'lambda', just use empty vector in rhs
    // but this will be used in computing the first set

    SCANEOF,

    primary,
    system_goal,
};

bool is_terminal_token(parser_token tok) {
    return tok <= ID || tok == SCANEOF || tok == LAMBDA;
}
