#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "node.h"
#include "symbol.h"
#include "type.h"
#include "ir.h"

/************************
 * CREATE IR STRUCTURES *
 ************************/

/*
 * An IR section is just a list of IR instructions. Each node has an associated
 * IR section if any code is required to implement it.
 */
static struct ir_section *ir_section(struct ir_instruction *first, struct ir_instruction *last) {
  struct ir_section *code;
  code = malloc(sizeof(struct ir_section));
  assert(NULL != code);

  code->first = first;
  code->last = last;
  return code;
}

static struct ir_section *ir_copy(struct ir_section *orig) {
  return ir_section(orig->first, orig->last);
}

/*
 * This joins two IR sections together into a new IR section.
 */
static struct ir_section *ir_concatenate(struct ir_section *before, struct ir_section *after) {
  /* patch the two sections together */
  before->last->next = after->first;
  after->first->prev = before->last;

  return ir_section(before->first, after->last);
}

static struct ir_section *ir_append(struct ir_section *section,
                                                           struct ir_instruction *instruction) {
  if (NULL == section) {
    section = ir_section(instruction, instruction);

  } else if (NULL == section->first || NULL == section->last) {
    assert(NULL == section->first && NULL == section->last);
    section->first = instruction;
    section->last = instruction;
    instruction->prev = NULL;
    instruction->next = NULL;

  } else {
    instruction->next = section->last->next;
    if (NULL != instruction->next) {
      instruction->next->prev = instruction;
    }
    section->last->next = instruction;

    instruction->prev = section->last;
    section->last = instruction;
  }
  return section;
}

/*
 * An IR instruction represents a single 3-address statement.
 */
static struct ir_instruction *ir_instruction(enum ir_instruction_kind kind) {
  struct ir_instruction *instruction;

  instruction = malloc(sizeof(struct ir_instruction));
  assert(NULL != instruction);

  instruction->kind = kind;

  instruction->next = NULL;
  instruction->prev = NULL;

  return instruction;
}

static void ir_operand_number(struct ir_instruction *instruction, int position, struct node *number) {
  instruction->operands[position].kind = OPERAND_NUMBER;
  instruction->operands[position].data.number = number->data.number.value;
}

static void ir_operand_temporary(struct ir_instruction *instruction, int position) {
  static int next_temporary;
  instruction->operands[position].kind = OPERAND_TEMPORARY;
  instruction->operands[position].data.temporary = next_temporary++;
}

static void ir_operand_copy(struct ir_instruction *instruction, int position, struct ir_operand *operand) {
  instruction->operands[position] = *operand;
}

/*******************************
 * GENERATE IR FOR EXPRESSIONS *
 *******************************/
static void ir_generate_for_number(struct node *number) {
  struct ir_instruction *instruction;
  assert(NODE_NUMBER == number->kind);

  instruction = ir_instruction(IR_LOAD_IMMEDIATE);
  ir_operand_temporary(instruction, 0);
  ir_operand_number(instruction, 1, number);

  number->ir = ir_section(instruction, instruction);

  number->data.number.result.ir_operand = &instruction->operands[0];
}

static void ir_generate_for_identifier(struct node *identifier) {
  struct ir_instruction *instruction;
  assert(NODE_IDENTIFIER == identifier->kind);
  instruction = ir_instruction(IR_NO_OPERATION);
  identifier->ir = ir_section(instruction, instruction);
  assert(NULL != identifier->data.identifier.symbol->result.ir_operand);
}

static void ir_generate_for_expression(struct node *expression);

static void ir_generate_for_arithmetic_binary_operation(enum ir_instruction_kind kind, struct node *binary_operation) {
  struct ir_instruction *instruction;
  assert(NODE_BINARY_OPERATION == binary_operation->kind);

  ir_generate_for_expression(binary_operation->data.binary_operation.left_operand);
  ir_generate_for_expression(binary_operation->data.binary_operation.right_operand);

  instruction = ir_instruction(kind);
  ir_operand_temporary(instruction, 0);
  ir_operand_copy(instruction, 1, node_get_result(binary_operation->data.binary_operation.left_operand)->ir_operand);
  ir_operand_copy(instruction, 2, node_get_result(binary_operation->data.binary_operation.right_operand)->ir_operand);

  binary_operation->ir = ir_concatenate(binary_operation->data.binary_operation.left_operand->ir,
                                        binary_operation->data.binary_operation.right_operand->ir);
  ir_append(binary_operation->ir, instruction);
  binary_operation->data.binary_operation.result.ir_operand = &instruction->operands[0];
}

static void ir_generate_for_simple_assignment(struct node *binary_operation) {
  struct ir_instruction *instruction;
  struct node *left;
  assert(NODE_BINARY_OPERATION == binary_operation->kind);

  ir_generate_for_expression(binary_operation->data.binary_operation.right_operand);

  left = binary_operation->data.binary_operation.left_operand;
  assert(NODE_IDENTIFIER == left->kind);

  instruction = ir_instruction(IR_COPY);
  if (NULL == left->data.identifier.symbol->result.ir_operand) {
    ir_operand_temporary(instruction, 0);
    left->data.identifier.symbol->result.ir_operand = &instruction->operands[0];
  } else {
    ir_operand_copy(instruction, 0, left->data.identifier.symbol->result.ir_operand);
  }
  ir_operand_copy(instruction, 1, node_get_result(binary_operation->data.binary_operation.right_operand)->ir_operand);

  binary_operation->ir = ir_copy(binary_operation->data.binary_operation.right_operand->ir);
  ir_append(binary_operation->ir, instruction);

  binary_operation->data.binary_operation.result.ir_operand = &instruction->operands[0];
}

static void ir_generate_for_binary_operation(struct node *binary_operation) {
  assert(NODE_BINARY_OPERATION == binary_operation->kind);

  switch (binary_operation->data.binary_operation.operation) {
    case BINOP_MULTIPLICATION:
      ir_generate_for_arithmetic_binary_operation(IR_MULTIPLY, binary_operation);
      break;

    case BINOP_DIVISION:
      ir_generate_for_arithmetic_binary_operation(IR_DIVIDE, binary_operation);
      break;

    case BINOP_ADDITION:
      ir_generate_for_arithmetic_binary_operation(IR_ADD, binary_operation);
      break;

    case BINOP_SUBTRACTION:
      ir_generate_for_arithmetic_binary_operation(IR_SUBTRACT, binary_operation);
      break;

    case BINOP_ASSIGN:
      ir_generate_for_simple_assignment(binary_operation);
      break;

    default:
      assert(0);
      break;
  }
}

static void ir_generate_for_expression(struct node *expression) {
  switch (expression->kind) {
    case NODE_IDENTIFIER:
      ir_generate_for_identifier(expression);
      break;

    case NODE_NUMBER:
      ir_generate_for_number(expression);
      break;

    case NODE_BINARY_OPERATION:
      ir_generate_for_binary_operation(expression);
      break;

    default:
      assert(0);
      break;
  }
}

static void ir_generate_for_expression_statement(struct node *expression_statement) {
  struct ir_instruction *instruction;
  struct node *expression = expression_statement->data.expression_statement.expression;
  assert(NODE_EXPRESSION_STATEMENT == expression_statement->kind);
  ir_generate_for_expression(expression);

  instruction = ir_instruction(IR_PRINT_NUMBER);
  ir_operand_copy(instruction, 0, node_get_result(expression)->ir_operand);

  expression_statement->ir = ir_copy(expression_statement->data.expression_statement.expression->ir);
  ir_append(expression_statement->ir, instruction);
}

int ir_generate_for_statement_list(struct node *statement_list) {
  struct node *init = statement_list->data.statement_list.init;
  struct node *statement = statement_list->data.statement_list.statement;

  assert(NODE_STATEMENT_LIST == statement_list->kind);

  if (NULL != init) {
    ir_generate_for_statement_list(init);
    ir_generate_for_expression_statement(statement);
    statement_list->ir = ir_concatenate(init->ir, statement->ir);
  } else {
    ir_generate_for_expression_statement(statement);
    statement_list->ir = statement->ir;
  }
  return 0;
}

/***********************
 * PRINT IR STRUCTURES *
 ***********************/

static void ir_print_opcode(FILE *output, enum ir_instruction_kind kind) {
  static char const * const instruction_names[] = {
    "NOP",
    "MULT",
    "DIV",
    "ADD",
    "SUB",
    "LI",
    "COPY",
    "PNUM",
    NULL
  };

  fprintf(output, "%-8s", instruction_names[kind]);
}

static void ir_print_operand(FILE *output, struct ir_operand *operand) {
  switch (operand->kind) {
    case OPERAND_NUMBER:
      fprintf(output, "%10hu", (unsigned short)operand->data.number);
      break;

    case OPERAND_TEMPORARY:
      fprintf(output, "     t%04d", operand->data.temporary);
      break;
  }
}
static void ir_print_instruction(FILE *output, struct ir_instruction *instruction) {
  ir_print_opcode(output, instruction->kind);

  switch (instruction->kind) {
    case IR_MULTIPLY:
    case IR_DIVIDE:
    case IR_ADD:
    case IR_SUBTRACT:
      ir_print_operand(output, &instruction->operands[0]);
      fprintf(output, ", ");
      ir_print_operand(output, &instruction->operands[1]);
      fprintf(output, ", ");
      ir_print_operand(output, &instruction->operands[2]);
      break;
    case IR_LOAD_IMMEDIATE:
    case IR_COPY:
      ir_print_operand(output, &instruction->operands[0]);
      fprintf(output, ", ");
      ir_print_operand(output, &instruction->operands[1]);
      break;
    case IR_PRINT_NUMBER:
      ir_print_operand(output, &instruction->operands[0]);
      break;
    case IR_NO_OPERATION:
      break;
    default:
      assert(0);
      break;
  }
}

void ir_print_section(FILE *output, struct ir_section *section) {
  int i = 0;
  struct ir_instruction *iter = section->first;
  struct ir_instruction *prev = NULL;
  while (NULL != iter && section->last != prev) {
    fprintf(output, "%5d     ", i++);
    ir_print_instruction(output, iter);
    fprintf(output, "\n");

    iter = iter->next;
  }
}
