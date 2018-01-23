#ifndef _PARSER_H
#define _PARSER_H

#include "scanner.yy.h"

struct node *parser_create_tree(int *error_count, yyscan_t scanner);

#endif
