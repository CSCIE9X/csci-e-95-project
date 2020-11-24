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
static void ir_generate_for_arithmetic_binary_operation(struct ir_context *context, enum ir_instruction_kind kind, struct node *binary_operation) {
  struct ir_instruction *instruction;
  assert(NODE_BINARY_OPERATION == binary_operation->kind);

    ir_ast_traversal(context, binary_operation->data.binary_operation.left_operand);
    ir_ast_traversal(context, binary_operation->data.binary_operation.right_operand);

  instruction = ir_instruction(kind);
  ir_operand_temporary(instruction, 0);
  ir_operand_copy(instruction, 1, node_get_result(binary_operation->data.binary_operation.left_operand)->ir_operand);
  ir_operand_copy(instruction, 2, node_get_result(binary_operation->data.binary_operation.right_operand)->ir_operand);

  binary_operation->ir = ir_concatenate(binary_operation->data.binary_operation.left_operand->ir,
                                        binary_operation->data.binary_operation.right_operand->ir);
  ir_append(binary_operation->ir, instruction);
  binary_operation->data.binary_operation.result.ir_operand = &instruction->operands[0];
}

static void ir_generate_for_simple_assignment(struct ir_context *context, struct node *binary_operation) {
  struct ir_instruction *instruction;
  struct node *left;
  assert(NODE_BINARY_OPERATION == binary_operation->kind);

    ir_ast_traversal(context, binary_operation->data.binary_operation.right_operand);

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

void ir_ast_traversal(struct ir_context *context, struct node * node) {
    if (!node) return;
    switch (node->kind) {
        case NODE_BINARY_OPERATION: {
            switch (node->data.binary_operation.operation) {
                case BINOP_MULTIPLICATION:
                    ir_generate_for_arithmetic_binary_operation(context, IR_MULTIPLY, node);
                    break;

                case BINOP_DIVISION:
                    ir_generate_for_arithmetic_binary_operation(context, IR_DIVIDE, node);
                    break;

                case BINOP_ADDITION:
                    ir_generate_for_arithmetic_binary_operation(context, IR_ADD, node);
                    break;

                case BINOP_SUBTRACTION:
                    ir_generate_for_arithmetic_binary_operation(context, IR_SUBTRACT, node);
                    break;

                case BINOP_ASSIGN:
                    ir_generate_for_simple_assignment(context, node);
                    break;
                default:
                    assert("unhandled binop in ir generation" && 0);
            }
            break;
        }
        case NODE_ERROR_STATEMENT: {
            assert("shouldn't progress to types if there are errors in the parse tree" && 0);
        }
        case NODE_EXPRESSION_STATEMENT: {
            struct ir_instruction *instruction;
            struct node *expression = node->data.expression_statement.expression;
            assert(NODE_EXPRESSION_STATEMENT == node->kind);
            ir_ast_traversal(context, expression);

            instruction = ir_instruction(IR_PRINT_NUMBER);
            ir_operand_copy(instruction, 0, node_get_result(expression)->ir_operand);

            node->ir = ir_copy(node->data.expression_statement.expression->ir);
            ir_append(node->ir, instruction);
            break;
        }
        case NODE_IDENTIFIER: {
            struct ir_instruction *instruction;
            assert(NODE_IDENTIFIER == node->kind);
            instruction = ir_instruction(IR_NO_OPERATION);
            node->ir = ir_section(instruction, instruction);
            assert(NULL != node->data.identifier.symbol->result.ir_operand);
            break;
        }
        case NODE_NUMBER: {
            struct ir_instruction *instruction;
            assert(NODE_NUMBER == node->kind);

            instruction = ir_instruction(IR_LOAD_IMMEDIATE);
            ir_operand_temporary(instruction, 0);
            ir_operand_number(instruction, 1, node);

            node->ir = ir_section(instruction, instruction);

            node->data.number.result.ir_operand = &instruction->operands[0];
            break;
        }
        case NODE_STATEMENT_LIST: {
            struct node *init = node->data.statement_list.init;
            struct node *statement = node->data.statement_list.statement;

            if (NULL != init) {
                ir_ast_traversal(context, init);
                ir_ast_traversal(context, statement);
                node->ir = ir_concatenate(init->ir, statement->ir);
            } else {
                ir_ast_traversal(context, statement);
                node->ir = statement->ir;
            }
            break;
        }
    }
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
      assert("unhandled ir for printing" && 0);
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
