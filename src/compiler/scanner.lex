%option yylineno
%option reentrant
%option bison-bridge
%option bison-locations
%option nounput
%option noyyalloc
%option noyyrealloc
%option noyyfree

%{
/*
 * scanner.lex
 *
 * This file contains the specification for the Flex generated scanner
 * for the CSCI E-95 sample language.
 *
 */

  #include <stdlib.h>
  #include <errno.h>
  #include <string.h>

  /* Suppress compiler warnings about unused variables and functions. */
  #define YY_EXIT_FAILURE ((void)yyscanner, 2)
  #define YY_NO_INPUT

  /* Track locations. */
  #define YY_EXTRA_TYPE int
  #define YY_USER_ACTION { yylloc->first_line = yylloc->last_line = yylineno; \
                           yylloc->first_column = yyextra; \
                           yylloc->last_column = yyextra + yyleng - 1; \
                           yyextra += yyleng; }

  #include "compiler.h"
  #include "parser.tab.h"
  #include "node.h"
  #include "type.h"
%}

newline         \n
ws              [ \t\v\f]

digit           [[:digit:]]
letter          [[:alpha:]]

id              {letter}({letter}|{digit})*
number          {digit}+

%%

{newline}   yyextra = 1;
{ws}        /* do nothing */

  /* operators begin */
\*          return ASTERISK;
\+          return PLUS;
-           return MINUS;
\/          return SLASH;
\=          return EQUAL;

\(          return LEFT_PAREN;
\)          return RIGHT_PAREN;
;           return SEMICOLON;
  /* operators end */

  /* constants begin */
{number}    *yylval = node_number(*yylloc, yytext); return NUMBER;
  /* constants end */

  /* identifiers */
{id}        *yylval = node_identifier(*yylloc, yytext, yyleng); return IDENTIFIER;

.           return -1;

%%

void scanner_initialize(yyscan_t *scanner, FILE *input) {
  yylex_init(scanner);
  yyset_in(input, *scanner);
  yyset_extra(1, *scanner);
}

void scanner_destroy(yyscan_t *scanner) {
  yylex_destroy(*scanner);
  scanner = NULL;
}

void scanner_print_tokens(FILE *output, int *error_count, yyscan_t scanner) {
  YYSTYPE val;
  YYLTYPE loc;
  int token;

  token = yylex(&val, &loc, scanner);
  while (0 != token) {
    /*
     * Print the line number. Use printf formatting and tabs to keep columns
     * lined up.
     */
    fprintf(output, "loc = %04d:%04d-%04d:%04d",
            loc.first_line, loc.first_column, loc.last_line, loc.last_column);

    /*
     * Print the scanned text. Try to use formatting but give up instead of
     * truncating if the text is too long.
     */
    if (yyget_leng(scanner) <= 20) {
      fprintf(output, "     text = %-20s", yyget_text(scanner));
    } else {
      fprintf(output, "     text = %s", yyget_text(scanner));
    }

    if (token <= 0) {
      fputs("     token = ERROR", output);
      (*error_count)++;
    } else {
      fprintf(output, "     token = [%-20s]", parser_token_name(token));

      switch (token) {
        case NUMBER:
          /* Print the type and value. */
          fputs("     type = ", output);
          type_print(output, val->data.number.result.type);
          fprintf(output, "     value = [%-10lu]", val->data.number.value);
          if (val->data.number.overflow) {
            fputs("     OVERFLOW", output);
            (*error_count)++;
          }
          break;

        case IDENTIFIER:
          fprintf(output, "     name = %s", val->data.identifier.name);
          break;
      }
    }
    fputs("\n", output);
    token = yylex(&val, &loc, scanner);
  }
}

/* Suppress compiler warnings about unused parameters. */
void *yyalloc (yy_size_t size, yyscan_t yyscanner __attribute__((unused)))
{
  return (void *)malloc(size);
}

/* Suppress compiler warnings about unused parameters. */
void *yyrealloc  (void *ptr, yy_size_t size, yyscan_t yyscanner __attribute__((unused)))
{
  return (void *)realloc((char *)ptr, size );
}

/* Suppress compiler warnings about unused parameters. */
void yyfree (void *ptr, yyscan_t yyscanner __attribute__((unused)))
{
  free((char *)ptr); /* see yyrealloc() for (char *) cast */
}
