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

void symbol_initialize_table(struct symbol_table *table);
int symbol_add_from_statement_list(struct symbol_table *table, struct node *statement_list);
void symbol_print_table(FILE *output, struct symbol_table *table);

#endif /* _SYMBOL_H */
