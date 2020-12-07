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

#define ENTERING true
#define EXITING false

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
    n->parent = NULL;
    n->puml_id = -1;

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
static const char *binary_operators[] = {
        "*",    /*  0 = BINOP_MULTIPLICATION */
        "/",    /*  1 = BINOP_DIVISION */
        "+",    /*  2 = BINOP_ADDITION */
        "-",    /*  3 = BINOP_SUBTRACTION */
        "=",    /*  4 = BINOP_ASSIGN */
        NULL
};

void print_ast_traversal(FILE *output, struct node *node) {
    assert(node);
    assert(output);
    switch (node->kind) {
        case NODE_BINARY_OPERATION: {
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

void * ast_traversal(
        void *context,
        struct node *node,
        traversal_callback callback) {
    assert(callback);
    if (!node) {
        return NULL;
    }
    void *left = NULL;
    void *right = NULL;
    callback(node, ENTERING, context, left, right);
    switch (node->kind) {
        case NODE_ERROR_STATEMENT:
        case NODE_IDENTIFIER:
        case NODE_NUMBER:
            break;
        case NODE_BINARY_OPERATION: {
            left = ast_traversal(context, node->data.binary_operation.left_operand, callback);
            right = ast_traversal(context, node->data.binary_operation.right_operand, callback);
            break;
        }
        case NODE_EXPRESSION_STATEMENT: {
            left = ast_traversal(context, node->data.expression_statement.expression, callback);
            break;
        }
        case NODE_STATEMENT_LIST: {
            left = ast_traversal(context, node->data.statement_list.init, callback);
            right = ast_traversal(context, node->data.statement_list.statement, callback);
            break;
        }
    }
    return callback(node, EXITING, context, left, right);
}

void * assign_parent(struct node *node, bool entering, void * __unused context, void * left, void * right) {
    if (entering) {
        return NULL;
    }
    node->parent = NULL;
    if (left) {
        struct node * left_child = left;
        left_child->parent = node;
    }
    if (right) {
        struct node * right_child = right;
        right_child->parent = node;
    }
    return node;
}

static void emitNodeDefinition(FILE *output, struct node *n, const char *unquotedNodeName) {
    fprintf(output, "( %s ) as (node_%d) %s\n", unquotedNodeName, n->puml_id, "");
}

static void emitNodeRelation(FILE *output, const char *relation, struct node *src, struct node *target) {
    fprintf(output, "(node_%d) --> (node_%d) : %s\n", src->puml_id, target->puml_id, relation);
}

void * puml_printer(struct node *node, bool entering, void * context) {
    if (!node) {
        return NULL;
    }
    struct puml_context * puml_context = context;
    if (entering) {
        node->puml_id = puml_context->next_id++;
    }
    if (entering && node->parent == NULL) {
        fprintf(puml_context->output, "@startuml\n");
    }
    switch (node->kind) {
        case NODE_NUMBER:
            if (entering) {
                fprintf(puml_context->output, "(%lu) as (node_%d) %s\n", node->data.number.value, node->puml_id, "");
            }
            break;
        case NODE_IDENTIFIER:
            if (entering) {
                emitNodeDefinition(puml_context->output, node, node->data.identifier.name);
            }
            break;
        case NODE_BINARY_OPERATION:
            if (!entering) {
                emitNodeDefinition(puml_context->output, node, binary_operators[node->data.binary_operation.operation]);
                emitNodeRelation(puml_context->output, "left", node, node->data.binary_operation.left_operand);
                emitNodeRelation(puml_context->output, "right", node, node->data.binary_operation.right_operand);
            }
            break;
        case NODE_EXPRESSION_STATEMENT:
            if (!entering) {
                emitNodeDefinition(puml_context->output, node, " ; ");
                emitNodeRelation(puml_context->output, "expr", node, node->data.expression_statement.expression);
            }
            break;
        case NODE_STATEMENT_LIST:
            if (!entering) {
                emitNodeDefinition(puml_context->output, node, "stmt_list");
                emitNodeRelation(puml_context->output, "init", node, node->data.statement_list.init);
                emitNodeRelation(puml_context->output, "stmt", node, node->data.statement_list.statement);
            }
            break;
        case NODE_ERROR_STATEMENT:
            if (entering) {
                emitNodeDefinition(puml_context->output, node, "error");
            }
            break;
    }
    if (!entering && node->parent == NULL) {
        fprintf(puml_context->output, "@enduml\n");
    }
    return NULL;
}
