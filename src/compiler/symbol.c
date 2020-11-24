#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "compiler.h"
#include "node.h"
#include "symbol.h"

/************************
 * CREATE SYMBOL TABLES *
 ************************/

void symbol_initialize_table(struct symbol_table *table) {
  table->variables = NULL;
}

static struct symbol *symbol_get(struct symbol_table *table, char name[]) {
  struct symbol_list *iter;
  for (iter = table->variables; NULL != iter; iter = iter->next) {
    if (!strcmp(name, iter->symbol.name)) {
      return &iter->symbol;
    }
  }
  return NULL;
}

static struct symbol *symbol_put(struct symbol_table *table, char name[]) {
  struct symbol_list *symbol_list;

  symbol_list = malloc(sizeof(struct symbol_list));
  assert(NULL != symbol_list);

  strncpy(symbol_list->symbol.name, name, IDENTIFIER_MAX);
  symbol_list->symbol.result.type = NULL;
  symbol_list->symbol.result.ir_operand = NULL;
  symbol_list->symbol.id = nextSymbolId++;

  symbol_list->next = table->variables;
  table->variables = symbol_list;

  return &symbol_list->symbol;
}

void symbol_ast_traversal(struct symbol_context *context, struct node * node) {
    if (!node) return;
    assert(context);
    assert(context->table);
    switch (node->kind) {
        case NODE_BINARY_OPERATION: {
            context->is_definition = node->data.binary_operation.left_operand && node->data.binary_operation.operation == BINOP_ASSIGN;
            symbol_ast_traversal(context, node->data.binary_operation.left_operand);
            context->is_definition = false;
            symbol_ast_traversal(context, node->data.binary_operation.right_operand);
            break;
        }
        case NODE_ERROR_STATEMENT: {
            assert("shouldn't progress to symbols if there are errors in the parse tree" && 0);
        }
        case NODE_EXPRESSION_STATEMENT: {
            symbol_ast_traversal(context, node->data.expression_statement.expression);
            break;
        }
        case NODE_IDENTIFIER: {
            node->data.identifier.symbol = symbol_get(context->table, node->data.identifier.name);
            if (NULL == node->data.identifier.symbol) {
                if (context->is_definition) {
                    node->data.identifier.symbol = symbol_put(context->table, node->data.identifier.name);
                } else {
                    context->error_count++;
                    compiler_print_error(node->location, "undefined identifier %s", node->data.identifier.name);
                }
            }
            break;
        }
        case NODE_NUMBER: {
            break;
        }
        case NODE_STATEMENT_LIST: {
            symbol_ast_traversal(context, node->data.statement_list.init);
            symbol_ast_traversal(context, node->data.statement_list.statement);
            break;
        }
    }
}

/***********************
 * PRINT SYMBOL TABLES *
 ***********************/

void symbol_print_table(FILE *output, struct symbol_table *table) {
  struct symbol_list *iter;

  fputs("symbol table:\n", output);

  for (iter = table->variables; NULL != iter; iter = iter->next) {
    fprintf(output, "  variable: %s /* %u */\n", iter->symbol.name, iter->symbol.id);
  }
  fputs("\n", output);
}
