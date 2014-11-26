#ifndef _SCANNER_H
#define _SCANNER_H

#include "parser.h"
#include "scanner.yy.h"

void scanner_initialize(yyscan_t *scanner, FILE *input);
void scanner_destroy(yyscan_t *scanner);
void scanner_print_tokens(FILE *output, int *error_count, yyscan_t scanner);

#endif
