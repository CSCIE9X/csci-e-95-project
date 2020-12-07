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

%token IDENTIFIER_T NUMBER_T
%token PLUS_T MINUS_T EQUAL_T
%token ASTERISK_T SLASH_T LEFT_PAREN_T RIGHT_PAREN_T SEMICOLON_T

%start program

%%

additive_expr
  : multiplicative_expr

  | additive_expr PLUS_T multiplicative_expr
          { $$ = node_binary_operation(@2, BINOP_ADDITION, $1, $3); }

  | additive_expr MINUS_T multiplicative_expr
          { $$ = node_binary_operation(@2, BINOP_SUBTRACTION, $1, $3); }
;

assignment_expr
  : additive_expr

  | primary_expr EQUAL_T additive_expr
          { $$ = node_binary_operation(@$, BINOP_ASSIGN, $1, $3); }
;

expr
  : assignment_expr
;

identifier
  : IDENTIFIER_T
;

multiplicative_expr
  : primary_expr

  | multiplicative_expr ASTERISK_T primary_expr
          { $$ = node_binary_operation(@2, BINOP_MULTIPLICATION, $1, $3); }

  | multiplicative_expr SLASH_T primary_expr
          { $$ = node_binary_operation(@2, BINOP_DIVISION, $1, $3); }
;

primary_expr
  : identifier

  | NUMBER_T

  | LEFT_PAREN_T expr RIGHT_PAREN_T
          { $$ = $2; }
;

program
  : statement_list
          { *root = $1; }
;

statement
  : expr SEMICOLON_T
          { $$ = node_expression_statement(@$, $1); }
  | error SEMICOLON_T
          { $$ = node_error_statement(@$); yyerrok; }
;


statement_list
  : statement
  | statement_list statement
          { $$ = node_statement_list(@$, $1, $2); }
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
    ast_traversal(NULL, parse_tree, &assign_parent);
    return parse_tree;
  }
}

char const *parser_token_name(int token) {
  return yytname[token - 255];
}

