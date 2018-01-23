#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>

#include "node.h"
#include "symbol.h"
#include "type.h"

/***************************
 * CREATE PARSE TREE NODES *
 ***************************/

/* Allocate and initialize a generic node. */
static struct node *node_create(enum node_kind kind, YYLTYPE location) {
  struct node *n;

  n = malloc(sizeof(struct node));
  assert(NULL != n);

  n->kind = kind;
  n->location = location;

  n->ir = NULL;
  return n;
}

/*
 * node_identifier - allocate a node to represent an identifier
 *
 * Parameters:
 *   text - string - contains the name of the identifier
 *   length - integer - the length of text (not including terminating NUL)
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_identifier(YYLTYPE location, char *text, int length)
{
  struct node *node = node_create(NODE_IDENTIFIER, location);
  memset(node->data.identifier.name, 0, IDENTIFIER_MAX + 1);
  strncpy(node->data.identifier.name, text, length);
  node->data.identifier.symbol = NULL;
  return node;
}

/*
 * node_number - allocate a node to represent a number
 *
 * Parameters:
 *   text - string - contains the numeric literal
 *   length - integer - the length of text (not including terminating NUL)
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 */
struct node *node_number(YYLTYPE location, char *text)
{
  struct node *node = node_create(NODE_NUMBER, location);
  errno = 0;
  node->data.number.value = strtoul(text, NULL, 10);
  if (node->data.number.value == ULONG_MAX && ERANGE == errno) {
    /* Strtoul indicated overflow. */
    node->data.number.overflow = true;
    node->data.number.result.type = type_basic(false, TYPE_BASIC_LONG);
  } else if (node->data.number.value > 0xFFFFFFFFul) {
    /* Value is too large for 32-bit unsigned long type. */
    node->data.number.overflow = true;
    node->data.number.result.type = type_basic(false, TYPE_BASIC_LONG);
  } else {
    node->data.number.overflow = false;
    node->data.number.result.type = type_basic(false, TYPE_BASIC_LONG);
  }

  node->data.number.result.ir_operand = NULL;
  return node;
}

struct node *node_binary_operation(YYLTYPE location,
                                   enum node_binary_operation operation,
                                   struct node *left_operand,
                                   struct node *right_operand)
{
  struct node *node = node_create(NODE_BINARY_OPERATION, location);
  node->data.binary_operation.operation = operation;
  node->data.binary_operation.left_operand = left_operand;
  node->data.binary_operation.right_operand = right_operand;
  node->data.binary_operation.result.type = NULL;
  node->data.binary_operation.result.ir_operand = NULL;
  return node;
}

struct node *node_expression_statement(YYLTYPE location, struct node *expression)
{
  struct node *node = node_create(NODE_EXPRESSION_STATEMENT, location);
  node->data.expression_statement.expression = expression;
  return node;
}

struct node *node_statement_list(YYLTYPE location,
                                 struct node *init,
                                 struct node *statement)
{
  struct node *node = node_create(NODE_STATEMENT_LIST, location);
  node->data.statement_list.init = init;
  node->data.statement_list.statement = statement;
  return node;
}

struct node *node_null_statement(YYLTYPE location)
{
  return node_create(NODE_NULL_STATEMENT, location);
}

struct result *node_get_result(struct node *expression) {
  switch (expression->kind) {
    case NODE_NUMBER:
      return &expression->data.number.result;
    case NODE_IDENTIFIER:
      return &expression->data.identifier.symbol->result;
    case NODE_BINARY_OPERATION:
      return &expression->data.binary_operation.result;
    default:
      assert(0);
      return NULL;
  }
}

/**************************
 * PRINT PARSE TREE NODES *
 **************************/

static void node_print_expression(FILE *output, struct node *expression);

static void node_print_binary_operation(FILE *output, struct node *binary_operation) {
  static const char *binary_operators[] = {
    "*",    /*  0 = BINOP_MULTIPLICATION */
    "/",    /*  1 = BINOP_DIVISION */
    "+",    /*  2 = BINOP_ADDITION */
    "-",    /*  3 = BINOP_SUBTRACTION */
    "=",    /*  4 = BINOP_ASSIGN */
    NULL
  };

  assert(NODE_BINARY_OPERATION == binary_operation->kind);

  fputs("(", output);
  node_print_expression(output, binary_operation->data.binary_operation.left_operand);
  fputs(" ", output);
  fputs(binary_operators[binary_operation->data.binary_operation.operation], output);
  fputs(" ", output);
  node_print_expression(output, binary_operation->data.binary_operation.right_operand);
  fputs(")", output);
}

static void node_print_number(FILE *output, struct node *number) {
  assert(NODE_NUMBER == number->kind);

  fprintf(output, "%lu", number->data.number.value);
}

/*
 * After the symbol table pass, we can print out the symbol address
 * for each identifier, so that we can compare instances of the same
 * variable and ensure that they have the same symbol.
 */
static void node_print_identifier(FILE *output, struct node *identifier) {
  assert(NODE_IDENTIFIER == identifier->kind);

  if (identifier->data.identifier.symbol) {
    fprintf(output, "%s /* %u */", identifier->data.identifier.name,
            identifier->data.identifier.symbol->id);
  } else {
    fprintf(output, "%s", identifier->data.identifier.name);
  }
}

static void node_print_expression(FILE *output, struct node *expression) {
  assert(NULL != expression);

  switch (expression->kind) {
    case NODE_BINARY_OPERATION:
      node_print_binary_operation(output, expression);
      break;
    case NODE_IDENTIFIER:
      node_print_identifier(output, expression);
      break;
    case NODE_NUMBER:
      node_print_number(output, expression);
      break;
    default:
      assert(0);
      break;
  }
}

static void node_print_expression_statement(FILE *output, struct node *expression_statement) {
  assert(NODE_EXPRESSION_STATEMENT == expression_statement->kind);

  node_print_expression(output, expression_statement->data.expression_statement.expression);

}

void node_print_statement_list(FILE *output, struct node *statement_list) {
  assert(NODE_STATEMENT_LIST == statement_list->kind);

  if (NULL != statement_list->data.statement_list.init) {
    node_print_statement_list(output, statement_list->data.statement_list.init);
  }
  node_print_expression_statement(output, statement_list->data.statement_list.statement);
  fputs(";\n", output);
}

