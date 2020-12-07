#ifndef _NODE_H
#define _NODE_H

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void *yyscan_t;
#endif

#include <stdio.h>
#include <stdbool.h>

#include "compiler.h"
#include "parser.tab.h"

struct type;

enum node_kind {
    NODE_NUMBER,
    NODE_IDENTIFIER,
    NODE_BINARY_OPERATION,
    NODE_EXPRESSION_STATEMENT,
    NODE_STATEMENT_LIST,
    NODE_ERROR_STATEMENT
};
struct node {
    enum node_kind kind;
    struct location location;
    struct ir_section *ir;
    union {
        struct {
            unsigned long value;
            bool overflow;
            struct result result;
        } number;
        struct {
            char name[IDENTIFIER_MAX + 1];
            struct symbol *symbol;
        } identifier;
        struct {
            int operation;
            struct node *left_operand;
            struct node *right_operand;
            struct result result;
        } binary_operation;
        struct {
            struct node *expression;
        } expression_statement;
        struct {
            struct node *init;
            struct node *statement;
        } statement_list;
    } data;
    // fields added to support puml visualization of the AST
    struct node *parent;
    int puml_id;
};

enum node_binary_operation {
    BINOP_MULTIPLICATION,
    BINOP_DIVISION,
    BINOP_ADDITION,
    BINOP_SUBTRACTION,
    BINOP_ASSIGN
};

/* Constructors */
struct node *node_number(YYLTYPE location, char *text);

struct node *node_identifier(YYLTYPE location, char *text, int length);

struct node *node_binary_operation(YYLTYPE location, enum node_binary_operation operation,
                                   struct node *left_operand, struct node *right_operand);

struct node *node_expression_statement(YYLTYPE location, struct node *expression);

struct node *node_statement_list(YYLTYPE location, struct node *init, struct node *statement);

struct node *node_error_statement(YYLTYPE location);

struct result *node_get_result(struct node *expression);

void print_ast_traversal(FILE *output, struct node *node);

typedef void *(*traversal_callback)(struct node *, bool, void *, void *, void *);

void *ast_traversal(void *context, struct node *node, traversal_callback callback);

void *assign_parent(struct node *node, bool entering, void *context, void *left, void *right);

struct puml_context {
    int next_id;
    FILE *output;
};

void *puml_printer(struct node *node, bool entering, void *context);

#endif
