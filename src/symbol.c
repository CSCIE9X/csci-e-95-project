#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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

static int symbol_add_from_identifier(struct symbol_table *table, struct node *identifier, bool define) {
  assert(NODE_IDENTIFIER == identifier->kind);

  identifier->data.identifier.symbol = symbol_get(table, identifier->data.identifier.name);
  if (NULL == identifier->data.identifier.symbol) {
    if (define) {
      identifier->data.identifier.symbol = symbol_put(table, identifier->data.identifier.name);
    } else {
      compiler_print_error(identifier->location, "undefined identifier %s", identifier->data.identifier.name);
      return 1;
    }
  }
  return 0;

}

static int symbol_add_from_expression(struct symbol_table *table, struct node *expression);

static int symbol_add_from_binary_operation(struct symbol_table *table, struct node *binary_operation) {
  assert(NODE_BINARY_OPERATION == binary_operation->kind);

  switch (binary_operation->data.binary_operation.operation) {
    case BINOP_MULTIPLICATION:
    case BINOP_DIVISION:
    case BINOP_ADDITION:
    case BINOP_SUBTRACTION:
      return symbol_add_from_expression(table, binary_operation->data.binary_operation.left_operand)
           + symbol_add_from_expression(table, binary_operation->data.binary_operation.right_operand);
    case BINOP_ASSIGN:
      if (NODE_IDENTIFIER == binary_operation->data.binary_operation.left_operand->kind) {
        return symbol_add_from_identifier(table, binary_operation->data.binary_operation.left_operand, true)
             + symbol_add_from_expression(table, binary_operation->data.binary_operation.right_operand);
      } else {
        compiler_print_error(binary_operation->data.binary_operation.left_operand->location,
                             "left operand of assignment must be an identifier");
        return 1
             + symbol_add_from_expression(table, binary_operation->data.binary_operation.left_operand)
             + symbol_add_from_expression(table, binary_operation->data.binary_operation.right_operand);
      }
    default:
      assert(0);
      return 1;
  }
}

static int symbol_add_from_expression(struct symbol_table *table, struct node *expression) {
  switch (expression->kind) {
    case NODE_BINARY_OPERATION:
      return symbol_add_from_binary_operation(table, expression);
    case NODE_IDENTIFIER:
      return symbol_add_from_identifier(table, expression, false);
    case NODE_NUMBER:
      return 0;
    default:
      assert(0);
      return 1;
  }
}

static int symbol_add_from_expression_statement(struct symbol_table *table, struct node *expression_statement) {
  assert(NODE_EXPRESSION_STATEMENT == expression_statement->kind);

  return symbol_add_from_expression(table, expression_statement->data.expression_statement.expression);
}

int symbol_add_from_statement_list(struct symbol_table *table, struct node *statement_list)
{
  int error_count = 0;
  assert(NODE_STATEMENT_LIST == statement_list->kind);

  if (NULL != statement_list->data.statement_list.init) {
    error_count += symbol_add_from_statement_list(table, statement_list->data.statement_list.init);
  }
  return error_count
       + symbol_add_from_expression_statement(table, statement_list->data.statement_list.statement);
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
