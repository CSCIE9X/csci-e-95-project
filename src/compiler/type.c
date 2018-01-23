#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "node.h"
#include "symbol.h"
#include "type.h"

/***************************
 * CREATE TYPE EXPRESSIONS *
 ***************************/

struct type *type_basic(bool is_unsigned, enum type_basic_kind datatype) {
  struct type *basic;

  basic = malloc(sizeof(struct type));
  assert(NULL != basic);

  basic->kind = TYPE_BASIC;
  basic->data.basic.is_unsigned = is_unsigned;
  basic->data.basic.datatype = datatype;
  return basic;
}

/****************************************
 * TYPE EXPRESSION INFO AND COMPARISONS *
 ****************************************/

static bool type_is_equal(struct type *left, struct type *right) {
  if (left->kind == right->kind) {
    switch (left->kind) {
      case TYPE_BASIC:
        return left->data.basic.is_unsigned == right->data.basic.is_unsigned
            && left->data.basic.datatype == right->data.basic.datatype;
      default:
        assert(0);
        break;
    }
  } else {
    return false;
  }
}

bool type_is_arithmetic(struct type *t) {
  return TYPE_BASIC == t->kind;
}

bool type_is_unsigned(struct type *t) {
  return type_is_arithmetic(t) && t->data.basic.is_unsigned;
}

int type_size(struct type *t) {
  switch (t->kind) {
    case TYPE_BASIC:
      switch (t->data.basic.datatype) {
        case TYPE_BASIC_CHAR:
          return 1;
        case TYPE_BASIC_SHORT:
          return 2;
        case TYPE_BASIC_INT:
          return 4;
        case TYPE_BASIC_LONG:
          return 4;
        default:
          assert(0);
          break;
      }
    case TYPE_POINTER:
      return 4;
    default:
      return 0;
  }
}

/*****************
 * TYPE CHECKING *
 *****************/

static void type_assign_in_expression(struct node *expression);

static void type_convert_usual_binary(struct node *binary_operation) {
  assert(NODE_BINARY_OPERATION == binary_operation->kind);
  assert(type_is_equal(node_get_result(binary_operation->data.binary_operation.left_operand)->type,
                       node_get_result(binary_operation->data.binary_operation.right_operand)->type));
  binary_operation->data.binary_operation.result.type =
    node_get_result(binary_operation->data.binary_operation.left_operand)->type;
}

static void type_convert_assignment(struct node *binary_operation) {
  assert(NODE_BINARY_OPERATION == binary_operation->kind);
  assert(type_is_equal(node_get_result(binary_operation->data.binary_operation.left_operand)->type,
                       node_get_result(binary_operation->data.binary_operation.right_operand)->type));
  binary_operation->data.binary_operation.result.type =
    node_get_result(binary_operation->data.binary_operation.left_operand)->type;
}

static void type_assign_in_binary_operation(struct node *binary_operation) {
  assert(NODE_BINARY_OPERATION == binary_operation->kind);
  type_assign_in_expression(binary_operation->data.binary_operation.left_operand);
  type_assign_in_expression(binary_operation->data.binary_operation.right_operand);

  switch (binary_operation->data.binary_operation.operation) {
    case BINOP_MULTIPLICATION:
    case BINOP_DIVISION:
    case BINOP_ADDITION:
    case BINOP_SUBTRACTION:
      type_convert_usual_binary(binary_operation);
      break;

    case BINOP_ASSIGN:
      type_convert_assignment(binary_operation);
      break;

    default:
      assert(0);
      break;
  }
}


static void type_assign_in_expression(struct node *expression) {
  switch (expression->kind) {
    case NODE_IDENTIFIER:
      if (NULL == expression->data.identifier.symbol->result.type) {
        expression->data.identifier.symbol->result.type = type_basic(false, TYPE_BASIC_INT);
      }
      break;

    case NODE_NUMBER:
      expression->data.number.result.type = type_basic(false, TYPE_BASIC_INT);
      break;

    case NODE_BINARY_OPERATION:
      type_assign_in_binary_operation(expression);
      break;
    default:
      assert(0);
      break;
  }
}

static void type_assign_in_expression_statement(struct node *expression_statement) {
  assert(NODE_EXPRESSION_STATEMENT == expression_statement->kind);
  type_assign_in_expression(expression_statement->data.expression_statement.expression);
}

int type_assign_in_statement_list(struct node *statement_list) {
  assert(NODE_STATEMENT_LIST == statement_list->kind);
  if (NULL != statement_list->data.statement_list.init) {
    type_assign_in_statement_list(statement_list->data.statement_list.init);
  }
  type_assign_in_expression_statement(statement_list->data.statement_list.statement);
  return 0;
}


/**************************
 * PRINT TYPE EXPRESSIONS *
 **************************/

static void type_print_basic(FILE *output, struct type *basic) {
  assert(TYPE_BASIC == basic->kind);

  if (basic->data.basic.is_unsigned) {
    fputs("unsigned", output);
  } else {
    fputs("  signed", output);
  }

  switch (basic->data.basic.datatype) {
    case TYPE_BASIC_INT:
      fputs("  int", output);
      break;
    case TYPE_BASIC_LONG:
      fputs(" long", output);
      break;
    default:
      assert(0);
      break;
  }
}

void type_print(FILE *output, struct type *kind) {
  assert(NULL != kind);

  switch (kind->kind) {
    case TYPE_BASIC:
      type_print_basic(output, kind);
      break;
    default:
      assert(0);
      break;
  }
}
