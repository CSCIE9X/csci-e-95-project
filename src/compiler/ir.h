#ifndef _IR_H
#define _IR_H

#include <stdio.h>
#include <stdbool.h>

struct node;
struct symbol;
struct symbol_table;

enum ir_operand_kind {
  OPERAND_NUMBER,
  OPERAND_TEMPORARY
};
struct ir_operand {
  enum ir_operand_kind kind;

  union {
    unsigned long number;
    int temporary;
  } data;
};

enum ir_instruction_kind {
  IR_NO_OPERATION,
  IR_MULTIPLY,
  IR_DIVIDE,
  IR_ADD,
  IR_SUBTRACT,
  IR_LOAD_IMMEDIATE,
  IR_COPY,
  IR_PRINT_NUMBER
};
struct ir_instruction {
  enum ir_instruction_kind kind;
  struct ir_instruction *prev, *next;
  struct ir_operand operands[3];
};

struct ir_section {
  struct ir_instruction *first, *last;
};

int ir_generate_for_statement_list(struct node *statement_list);

void ir_print_section(FILE *output, struct ir_section *section);

#endif
