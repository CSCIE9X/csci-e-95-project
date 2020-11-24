//
// Created by Mark Ford on 11/22/20.
//
#define CTEST_MAIN

#define LEX_MACRO(input, expected) \
    do { \
    FILE *in = fmemopen((void *) input, strlen(input), "r"); \
    scanner_initialize(&data->scanner, in); \
    int actual = yylex(&data->val, &data->loc, data->scanner); \
    ASSERT_EQUAL(expected, actual);\
    fclose(in);                    \
    } while(0)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ctest.h"
#include "compiler.h"
#include "node.h"
#include "scanner.h"
#include "parser.tab.h"
#include "symbol.h"

int main(int argc, char **argv) {
    ctest_main(argc, (const char **) argv);
    return 0;
}

static struct node * parse_helper(char *input) {
    FILE *in = fmemopen((void *) input, strlen(input), "r");
    yyscan_t scanner;
    scanner_initialize(&scanner, in);
    int error_count = 0;
    struct node *parse_tree = parser_create_tree(&error_count, scanner);
    fclose(in);
    scanner_destroy(&scanner);
    ASSERT_EQUAL(0, error_count);
    return parse_tree;

}

static void print_and_assert(struct node *parse_tree, char *expected, char* buff, int buffLen) {
    int fds[2];
    pipe(fds);

    FILE *readMe = fdopen(fds[0], "r");
    FILE *writeMe = fdopen(fds[1], "w");

    node_print_any(writeMe, parse_tree);
    fclose(writeMe);

    fread(buff, sizeof(char), buffLen, readMe);
    fclose(readMe);
    ASSERT_STR(expected, buff);
}

CTEST_DATA(scanner_tests) {
    yyscan_t scanner;
    YYSTYPE val;
    YYLTYPE loc;
    int bufferLength;
    char* buffer;
};

CTEST_SETUP(scanner_tests) {
    data->bufferLength = 1024;
    data->buffer = (char*)malloc(data->bufferLength);
    data->val = NULL;
}

CTEST_TEARDOWN(scanner_tests) {
    if (data->scanner) {
        scanner_destroy(&data->scanner);
    }
    if (data->buffer) free(data->buffer);
}

CTEST2(scanner_tests, plus_t) {
    LEX_MACRO("+", PLUS_T);
}
CTEST2(scanner_tests, asterisk_t) {
    LEX_MACRO("*", ASTERISK_T);
}
CTEST2(scanner_tests, slash_t) {
    LEX_MACRO("/", SLASH_T);
}
CTEST2(scanner_tests, number_t) {
    LEX_MACRO("42", NUMBER_T);
    ASSERT_NOT_NULL(data->val);
    ASSERT_EQUAL(data->val->location.first_line, 1);
    ASSERT_EQUAL(42, data->val->data.number.value);
    ASSERT_EQUAL(false, data->val->data.number.overflow);
}
CTEST2(scanner_tests, identifier_t) {
    LEX_MACRO("abc", IDENTIFIER_T);
    ASSERT_NOT_NULL(data->val);
    ASSERT_EQUAL(data->val->location.first_line, 1);
    ASSERT_STR("abc", data->val->data.identifier.name);
    ASSERT_NULL(data->val->data.identifier.symbol);
}

CTEST_DATA(parser_tests) {
    int bufferLength;
    char* buffer;
};
CTEST_SETUP(parser_tests) {
    data->bufferLength = 1024;
    data->buffer = (char*)malloc(data->bufferLength);
}

CTEST_TEARDOWN(parser_tests) {
    if (data->buffer) free(data->buffer);
}
CTEST2(parser_tests, simple_expr) {
    struct node * parse_tree = parse_helper("2;");
    print_and_assert(parse_tree, "2;\n", data->buffer, data->bufferLength);
}
CTEST2(parser_tests, binop_plus) {
    struct node * parse_tree = parse_helper("2 + 5;");
    print_and_assert(parse_tree, "(2 + 5);\n", data->buffer, data->bufferLength);
}
CTEST2(parser_tests, binop_assign) {
    struct node * parse_tree = parse_helper("a = 4;");
    print_and_assert(parse_tree, "(a = 4);\n", data->buffer, data->bufferLength);
}
CTEST2(parser_tests, statement_list) {
    struct node * parse_tree = parse_helper("a = 4;\n2 + a;");
    print_and_assert(parse_tree,
                 "(a = 4);\n(2 + a);\n", data->buffer, data->bufferLength);
}

CTEST_DATA(symbol_tests) {
    int bufferLength;
    char* buffer;
    struct symbol_table symbol_table;
};
CTEST_SETUP(symbol_tests) {
    data->bufferLength = 1024;
    data->buffer = (char*)malloc(data->bufferLength);
    symbol_initialize_table(&data->symbol_table);
}

CTEST_TEARDOWN(symbol_tests) {
    if (data->buffer) free(data->buffer);
}

static int symbol_helper(char * source, struct symbol_table *symbol_table) {
    struct symbol_context context = {
            symbol_table, false, 0
    };
    struct node *parse_tree = parse_helper(source);
    symbol_add_from_any(&context, parse_tree);
    return context.error_count;
}

CTEST2(symbol_tests, simple_expr) {
    int errors = symbol_helper("2;", &data->symbol_table);
    ASSERT_EQUAL(0, errors);
}
CTEST2(symbol_tests, assign) {
    int errors = symbol_helper("a = 2;", &data->symbol_table);
    ASSERT_EQUAL(0, errors);
    ASSERT_NOT_NULL(data->symbol_table.variables);
    ASSERT_STR("a", data->symbol_table.variables->symbol.name);
    ASSERT_NULL(data->symbol_table.variables->next);
}
CTEST2(symbol_tests, stmt_list) {
    char * source = "a = 2; b = a + 4;";
    int errors = symbol_helper(source, &data->symbol_table);
    ASSERT_EQUAL(0, errors);
}
CTEST2(symbol_tests, error_undefined) {
    int errors = symbol_helper("a + 2;", &data->symbol_table);
    ASSERT_EQUAL(1, errors);
}
