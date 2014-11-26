#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "type.h"
#include "symbol.h"
#include "ir.h"
#include "mips.h"

#define REG_EXHAUSTED   -1

#define FIRST_USABLE_REGISTER  8
#define LAST_USABLE_REGISTER  23
#define NUM_REGISTERS         32


/****************************
 * MIPS TEXT SECTION OUTPUT *
 ****************************/

void mips_print_temporary_operand(FILE *output, struct ir_operand *operand) {
  assert(OPERAND_TEMPORARY == operand->kind);

  fprintf(output, "%8s%02d", "$", operand->data.temporary + FIRST_USABLE_REGISTER);
}

void mips_print_number_operand(FILE *output, struct ir_operand *operand) {
  assert(OPERAND_NUMBER == operand->kind);

  fprintf(output, "%10lu", operand->data.number);
}

void mips_print_arithmetic(FILE *output, struct ir_instruction *instruction) {
  static char *opcodes[] = {
    NULL,
    "mulu",
    "divu",
    "addu",
    "subu",
    NULL
  };
  fprintf(output, "%10s ", opcodes[instruction->kind]);
  mips_print_temporary_operand(output, &instruction->operands[0]);
  fputs(", ", output);
  mips_print_temporary_operand(output, &instruction->operands[1]);
  fputs(", ", output);
  mips_print_temporary_operand(output, &instruction->operands[2]);
  fputs("\n", output);
}

void mips_print_copy(FILE *output, struct ir_instruction *instruction) {
  fprintf(output, "%10s ", "or");
  mips_print_temporary_operand(output, &instruction->operands[0]);
  fputs(", ", output);
  mips_print_temporary_operand(output, &instruction->operands[1]);
  fprintf(output, ", %10s\n", "$0");
}

void mips_print_load_immediate(FILE *output, struct ir_instruction *instruction) {
  fprintf(output, "%10s ", "li");
  mips_print_temporary_operand(output, &instruction->operands[0]);
  fputs(", ", output);
  mips_print_number_operand(output, &instruction->operands[1]);
  fputs("\n", output);
}

void mips_print_print_number(FILE *output, struct ir_instruction *instruction) {
  /* Print the number. */
  fprintf(output, "%10s %10s, %10s, %10d\n", "ori", "$v0", "$0", 1);
  fprintf(output, "%10s %10s, %10s, ", "or", "$a0", "$0");
  mips_print_temporary_operand(output, &instruction->operands[0]);
  fprintf(output, "\n%10s\n", "syscall");

  /* Print a newline. */
  fprintf(output, "%10s %10s, %10s, %10d\n", "ori", "$v0", "$0", 4);
  fprintf(output, "%10s %10s, %10s", "la", "$a0", "newline");
  fprintf(output, "\n%10s\n", "syscall");
}

void mips_print_instruction(FILE *output, struct ir_instruction *instruction) {
  switch (instruction->kind) {
    case IR_MULTIPLY:
    case IR_DIVIDE:
    case IR_ADD:
    case IR_SUBTRACT:
      mips_print_arithmetic(output, instruction);
      break;

    case IR_COPY:
      mips_print_copy(output, instruction);
      break;

    case IR_LOAD_IMMEDIATE:
      mips_print_load_immediate(output, instruction);
      break;

    case IR_PRINT_NUMBER:
      mips_print_print_number(output, instruction);
      break;

    case IR_NO_OPERATION:
      break;

    default:
      assert(0);
      break;
  }
}

void mips_print_text_section(FILE *output, struct ir_section *section) {
  struct ir_instruction *instruction;

  fputs("\n.data\nnewline: .asciiz \"\\n\"", output);
  fputs("\n.text\nmain:\n", output);

  for (instruction = section->first; instruction != section->last->next; instruction = instruction->next) {
    mips_print_instruction(output, instruction);
  }

  /* Return from main. */
  fprintf(output, "\n%10s %10s\n", "jr", "$ra");
}

void mips_print_program(FILE *output, struct ir_section *section) {
  mips_print_text_section(output, section);
}
