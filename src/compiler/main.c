#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <assert.h>

#include "compiler.h" // include is used, it provides def for YYSTYPE
#include "parser.h"
#include "scanner.h"
#include "node.h"
#include "symbol.h"
#include "type.h"
#include "ir.h"
#include "mips.h"

#define DEBUG 0


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
 * -O : the optimization level. Defaults to 0 meaning no optimization is performed
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
    int optlevel = 0;

    strncpy(output_name, "output.s", NAME_MAX + 1);
    stage = "mips";
    while (-1 != (opt = getopt(argc, argv, "O:o:s:"))) {
        switch (opt) {
            case 'O':
                optlevel = (int) strtol(optarg, (char **)NULL, 10);
                break;
            case 'o':
                strncpy(output_name, optarg, NAME_MAX);
                break;
            case 's':
                stage = optarg;
                break;
        }
    }

    assert(optlevel >= 0);

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
        print_ast_traversal(stdout, parse_tree);
        return 0;
    }

    if (0 == strcmp("puml", stage)) {
        struct puml_context context = {
                0, stdout
        };
        ast_traversal(&context, parse_tree, (traversal_callback) puml_printer);
        return 0;
    }

    symbol_initialize_table(&symbol_table);
    error_count = 0;
    struct symbol_context context = {
            &symbol_table, false, error_count
    };
    symbol_ast_traversal(&context, parse_tree);
    error_count = context.error_count;
    if (error_count > 0) {
        print_errors_from_pass("Symbol table", error_count);
        return 1;
    }
    if (0 == strcmp("symbol", stage)) {
        fprintf(stdout, "================= SYMBOLS ================\n");
        symbol_print_table(stdout, &symbol_table);
        fprintf(stdout, "=============== PARSE TREE ===============\n");
        print_ast_traversal(stdout, parse_tree);
        return 0;
    }

    struct type_context type_context = {0};
    type_ast_traversal(&type_context, parse_tree);
    error_count = type_context.error_count;
    if (error_count > 0) {
        print_errors_from_pass("Type checking", error_count);
        return 1;
    }
    if (0 == strcmp("type", stage)) {
        print_ast_traversal(stdout, parse_tree);
        return 0;
    }

    struct ir_context ir_context = { 0 };
    ir_ast_traversal(&ir_context, parse_tree);
    error_count = ir_context.error_count;
    if (error_count > 0) {
        print_errors_from_pass("IR generation", error_count);
        return 1;
    }
    if (0 == strcmp("ir", stage)) {
        ir_print_section(stdout, parse_tree->ir);
        return 0;
    }

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
