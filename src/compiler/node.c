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
struct node *node_identifier(YYLTYPE location, char *text, int length) {
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
struct node *node_number(YYLTYPE location, char *text) {
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
                                   struct node *right_operand) {
    struct node *node = node_create(NODE_BINARY_OPERATION, location);
    node->data.binary_operation.operation = operation;
    node->data.binary_operation.left_operand = left_operand;
    node->data.binary_operation.right_operand = right_operand;
    node->data.binary_operation.result.type = NULL;
    node->data.binary_operation.result.ir_operand = NULL;
    return node;
}

struct node *node_expression_statement(YYLTYPE location, struct node *expression) {
    struct node *node = node_create(NODE_EXPRESSION_STATEMENT, location);
    node->data.expression_statement.expression = expression;
    return node;
}

struct node *node_statement_list(YYLTYPE location,
                                 struct node *init,
                                 struct node *statement) {
    struct node *node = node_create(NODE_STATEMENT_LIST, location);
    node->data.statement_list.init = init;
    node->data.statement_list.statement = statement;
    return node;
}

struct node *node_error_statement(YYLTYPE location) {
    return node_create(NODE_ERROR_STATEMENT, location);
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
            assert("unhandled kind in node_get_result" && 0);
    }
}

/**************************
 * PRINT PARSE TREE NODES *
 **************************/

void print_ast_traversal(FILE *output, struct node *node) {
    assert(node);
    assert(output);
    switch (node->kind) {
        case NODE_BINARY_OPERATION: {
            static const char *binary_operators[] = {
                    "*",    /*  0 = BINOP_MULTIPLICATION */
                    "/",    /*  1 = BINOP_DIVISION */
                    "+",    /*  2 = BINOP_ADDITION */
                    "-",    /*  3 = BINOP_SUBTRACTION */
                    "=",    /*  4 = BINOP_ASSIGN */
                    NULL
            };

            assert(NODE_BINARY_OPERATION == node->kind);

            fputs("(", output);
            print_ast_traversal(output, node->data.binary_operation.left_operand);
            fputs(" ", output);
            fputs(binary_operators[node->data.binary_operation.operation], output);
            fputs(" ", output);
            print_ast_traversal(output, node->data.binary_operation.right_operand);
            fputs(")", output);
            break;
        }
        case NODE_ERROR_STATEMENT: {
            fprintf(output, "error");
            break;
        }
        case NODE_EXPRESSION_STATEMENT: {
            print_ast_traversal(output, node->data.expression_statement.expression);
            fputs(";\n", output);
            break;
        }
        case NODE_IDENTIFIER: {
            if (node->data.identifier.symbol) {
                fprintf(output, "%s /* %u */", node->data.identifier.name,
                        node->data.identifier.symbol->id);
            } else {
                fprintf(output, "%s", node->data.identifier.name);
            }
            break;
        }
        case NODE_NUMBER: {
            fprintf(output, "%lu", node->data.number.value);
            break;
        }
        case NODE_STATEMENT_LIST: {
            if (NULL != node->data.statement_list.init) {
                print_ast_traversal(output, node->data.statement_list.init);
            }
            print_ast_traversal(output, node->data.statement_list.statement);
            break;
        }
    }
}

