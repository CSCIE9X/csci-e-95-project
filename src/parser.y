%verbose
%debug
%defines
%locations
%pure-parser
%lex-param {yyscan_t scanner}
%parse-param {YYSTYPE *root}
%parse-param {int *error_count}
%parse-param {yyscan_t scanner}
%token-table

%{

  #ifndef YY_TYPEDEF_YY_SCANNER_T
  #define YY_TYPEDEF_YY_SCANNER_T
  typedef void* yyscan_t;
  #endif

  #include <stdio.h>

  #include "compiler.h"
  #include "parser.tab.h"
  #include "scanner.yy.h"
  #include "node.h"

  #define YYERROR_VERBOSE
  static void yyerror(YYLTYPE *loc, YYSTYPE *root,
                      int *error_count, yyscan_t scanner,
                      char const *s);
%}

%token IDENTIFIER NUMBER STRING

%token BREAK CHAR CONTINUE DO ELSE FOR GOTO IF
%token INT LONG RETURN SHORT SIGNED UNSIGNED VOID WHILE

%token LEFT_PAREN RIGHT_PAREN LEFT_SQUARE RIGHT_SQUARE LEFT_CURLY RIGHT_CURLY

%token AMPERSAND ASTERISK CARET COLON COMMA EQUAL EXCLAMATION GREATER
%token LESS MINUS PERCENT PLUS SEMICOLON SLASH QUESTION TILDE VBAR

%token AMPERSAND_AMPERSAND AMPERSAND_EQUAL ASTERISK_EQUAL CARET_EQUAL
%token EQUAL_EQUAL EXCLAMATION_EQUAL GREATER_EQUAL GREATER_GREATER
%token GREATER_GREATER_EQUAL LESS_EQUAL LESS_LESS LESS_LESS_EQUAL
%token MINUS_EQUAL MINUS_MINUS PERCENT_EQUAL PLUS_EQUAL PLUS_PLUS
%token SLASH_EQUAL VBAR_EQUAL VBAR_VBAR

%start program

%%

additive_expr
  : multiplicative_expr

  | additive_expr PLUS multiplicative_expr
          { $$ = node_binary_operation(yylloc, BINOP_ADDITION, $1, $3); }

  | additive_expr MINUS multiplicative_expr
          { $$ = node_binary_operation(yylloc, BINOP_SUBTRACTION, $1, $3); }
;

assignment_expr
  : additive_expr

  | primary_expr EQUAL additive_expr
          { $$ = node_binary_operation(yylloc, BINOP_ASSIGN, $1, $3); }
;

expr
  : assignment_expr
;

identifier
  : IDENTIFIER
;

multiplicative_expr
  : primary_expr

  | multiplicative_expr ASTERISK primary_expr
          { $$ = node_binary_operation(yylloc, BINOP_MULTIPLICATION, $1, $3); }

  | multiplicative_expr SLASH primary_expr
          { $$ = node_binary_operation(yylloc, BINOP_DIVISION, $1, $3); }
;

primary_expr
  : identifier

  | NUMBER

  | LEFT_PAREN expr RIGHT_PAREN
          { $$ = $2; }
;

program
  : statement_list
          { *root = $1; }
;

statement
  : expr SEMICOLON
          { $$ = node_expression_statement(yylloc, $1); }
  | error SEMICOLON
          { $$ = node_null_statement(yylloc); yyerrok; }
;


statement_list
  : statement
          { $$ = node_statement_list(yylloc, NULL, $1); }
  | statement_list statement
          { $$ = node_statement_list(yylloc, $1, $2); }
;

%%

static void yyerror(YYLTYPE *loc,
                    YYSTYPE *root __attribute__((unused)),
                    int *error_count,
                    yyscan_t scanner __attribute__((unused)),
                    char const *s)
{
  compiler_print_error(*loc, s);
  (*error_count)++;
}

struct node *parser_create_tree(int *error_count, yyscan_t scanner) {
  struct node *parse_tree;
  int result = yyparse(&parse_tree, error_count, scanner);
  if (result == 1 || *error_count > 0) {
    return NULL;
  } else if (result == 2) {
    fprintf(stdout, "Parser ran out of memory.\n");
    return NULL;
  } else {
    return parse_tree;
  }
}

char const *parser_token_name(int token) {
  return yytname[token - 255];
}

