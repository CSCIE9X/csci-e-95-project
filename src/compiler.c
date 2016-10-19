#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <limits.h>
#include <stdarg.h>

#include "compiler.h"
#include "parser.h"
#include "scanner.h"
#include "node.h"
#include "symbol.h"
#include "type.h"
#include "ir.h"
#include "mips.h"

extern int errno;

void compiler_print_error(YYLTYPE location, const char *format, ...) {
  va_list ap;
  fprintf(stdout, "Error (%d, %d) to (%d, %d): ",
          location.first_line, location.first_column,
          location.last_line, location.last_column);
  va_start(ap, format);
  vfprintf(stdout, format, ap);
  va_end(ap);
  fputc('\n', stdout);
}

static void print_errors_from_pass(char *pass, int error_count) {
  fprintf(stdout, "%s encountered %d %s.\n",
          pass, error_count, (error_count == 1 ? "error" : "errors"));
}

/**
 * Launches the compiler.
 * 
 * The following describes the arguments to the program:
 * compiler [-s (scanner|parser|symbol|type|ir|mips)] [-o outputfile] [inputfile|stdin]
 *
 * -s : the name of the stage to stop after. Defaults to
 *      runs all of the stages.
 * -o : the name of the output file. Defaults to "output.s"      
 *
 * You should pass the name of the file to process or redirect stdin.
 */
int main(int argc, char **argv) {
  FILE *output;
  struct symbol_table symbol_table;
  char *stage, output_name[NAME_MAX + 1];
  int opt;
  yyscan_t scanner;
  struct node *parse_tree;
  int error_count;

  strncpy(output_name, "output.s", NAME_MAX + 1);
  stage = "mips";
  while (-1 != (opt = getopt(argc, argv, "o:s:"))) {
    switch (opt) {
      case 'o':
        strncpy(output_name, optarg, NAME_MAX);
        break;
      case 's':
        stage = optarg;
        break;
    }
  }

  /* Figure out whether we're using stdin/stdout or file in/file out. */
  if (optind >= argc) {
    scanner_initialize(&scanner, stdin);
  } else if (optind == argc - 1) {
    scanner_initialize(&scanner, fopen(argv[optind], "r"));
  } else {
    fprintf(stdout, "Expected 1 input file, found %d.\n", argc - optind);
    return 1;
  }

  if (0 == strcmp("scanner", stage)) {
    error_count = 0;
    scanner_print_tokens(stdout, &error_count, scanner);
    scanner_destroy(&scanner);
    if (error_count > 0) {
      print_errors_from_pass("Scanner", error_count);
      return 1;
    } else {
      return 0;
    }
  }

  error_count = 0;
  parse_tree = parser_create_tree(&error_count, scanner);
  scanner_destroy(&scanner);
  if (NULL == parse_tree) {
    print_errors_from_pass("Parser", error_count);
    return 1;
  }

  if (0 == strcmp("parser", stage)) {
    node_print_statement_list(stdout, parse_tree);
    return 0;
  }

  symbol_initialize_table(&symbol_table);
  error_count = symbol_add_from_statement_list(&symbol_table, parse_tree);
  if (error_count > 0) {
    print_errors_from_pass("Symbol table", error_count);
    return 1;
  }
  fprintf(stdout, "================= SYMBOLS ================\n");
  symbol_print_table(stdout, &symbol_table);
  if (0 == strcmp("symbol", stage)) {
    fprintf(stdout, "=============== PARSE TREE ===============\n");
    node_print_statement_list(stdout, parse_tree);
    return 0;
  }

  error_count = type_assign_in_statement_list(parse_tree);
  if (error_count > 0) {
    print_errors_from_pass("Type checking", error_count);
    return 1;
  }
  fprintf(stdout, "=============== PARSE TREE ===============\n");
  node_print_statement_list(stdout, parse_tree);
  if (0 == strcmp("type", stage)) {
    return 0;
  }

  error_count = ir_generate_for_statement_list(parse_tree);
  if (error_count > 0) {
    print_errors_from_pass("IR generation", error_count);
    return 1;
  }
  fprintf(stdout, "=================== IR ===================\n");
  ir_print_section(stdout, parse_tree->ir);
  if (0 == strcmp("ir", stage)) {
    return 0;
  }

  fprintf(stdout, "================== MIPS ==================\n");
  mips_print_program(stdout, parse_tree->ir);
  fputs("\n\n", stdout);

  output = fopen(output_name, "w");
  if (NULL == output) {
    fprintf(stdout, "Could not open output file %s: %s", optarg, strerror(errno));
    return 1;
  }
  mips_print_program(output, parse_tree->ir);
  fputs("\n\n", output);

  return 0;
}
