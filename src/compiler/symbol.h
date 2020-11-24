#ifndef _SYMBOL_H
#define _SYMBOL_H

#include <stdio.h>

#include "compiler.h"
struct node;
struct type;

unsigned int nextSymbolId;

struct symbol {
  char name[IDENTIFIER_MAX + 1];
  struct result result;
  unsigned int id;
};

struct symbol_list {
  struct symbol symbol;
  struct symbol_list *next;
};

struct symbol_table {
  struct symbol_list *variables;
};

struct symbol_context {
    struct symbol_table * table;
    bool is_definition;
    int error_count;
};

void symbol_initialize_table(struct symbol_table *table);
void symbol_ast_traversal(struct symbol_context *context, struct node *node);
void symbol_print_table(FILE *output, struct symbol_table *table);

#endif /* _SYMBOL_H */
