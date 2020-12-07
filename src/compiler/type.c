#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "node.h"
#include "symbol.h"
#include "type.h"

/***************************
 * CREATE TYPE EXPRESSIONS *
 ***************************/

struct type *type_basic(bool is_unsigned, enum type_basic_kind datatype) {
    struct type *basic;

    basic = malloc(sizeof(struct type));
    assert(NULL != basic);

    basic->kind = TYPE_BASIC;
    basic->data.basic.is_unsigned = is_unsigned;
    basic->data.basic.datatype = datatype;
    return basic;
}

/****************************************
 * TYPE EXPRESSION INFO AND COMPARISONS *
 ****************************************/

static bool type_is_equal(struct type *left, struct type *right) {
    if (left->kind == right->kind) {
        switch (left->kind) {
            case TYPE_BASIC:
                return left->data.basic.is_unsigned == right->data.basic.is_unsigned
                       && left->data.basic.datatype == right->data.basic.datatype;
            default:
                assert("only basic types support for calc" && 0);
        }
    } else {
        return false;
    }
}

/*****************
 * TYPE CHECKING *
 *****************/

static void type_convert_usual_binary(struct node *binary_operation) {
    assert(NODE_BINARY_OPERATION == binary_operation->kind);
    assert(type_is_equal(node_get_result(binary_operation->data.binary_operation.left_operand)->type,
                         node_get_result(binary_operation->data.binary_operation.right_operand)->type));
    binary_operation->data.binary_operation.result.type =
            node_get_result(binary_operation->data.binary_operation.left_operand)->type;
}

static void type_convert_assignment(struct node *binary_operation) {
    assert(NODE_BINARY_OPERATION == binary_operation->kind);
    assert(type_is_equal(node_get_result(binary_operation->data.binary_operation.left_operand)->type,
                         node_get_result(binary_operation->data.binary_operation.right_operand)->type));
    binary_operation->data.binary_operation.result.type =
            node_get_result(binary_operation->data.binary_operation.left_operand)->type;
}

void type_ast_traversal(struct type_context *context, struct node *node) {
    if (!node) return;
    switch (node->kind) {
        case NODE_BINARY_OPERATION: {
            type_ast_traversal(context, node->data.binary_operation.left_operand);
            type_ast_traversal(context, node->data.binary_operation.right_operand);
            switch (node->data.binary_operation.operation) {
                case BINOP_MULTIPLICATION:
                case BINOP_ADDITION:
                case BINOP_SUBTRACTION:
                    type_convert_usual_binary(node);
                    break;
                case BINOP_DIVISION:
                    type_convert_usual_binary(node);
                    if (node->data.binary_operation.right_operand->kind == NODE_NUMBER &&
                        node->data.binary_operation.right_operand->data.number.value == 0) {
                        context->error_count++;
                        compiler_print_error(node->location, "Division by zero");
                    }
                    break;
                case BINOP_ASSIGN:
                    type_convert_assignment(node);
                    break;
                default:
                    assert("unsupported binary op in assignment" && 0);
            }
            break;
        }
        case NODE_ERROR_STATEMENT: {
            assert("shouldn't progress to types if there are errors in the parse tree" && 0);
        }
        case NODE_EXPRESSION_STATEMENT: {
            type_ast_traversal(context, node->data.expression_statement.expression);
            break;
        }
        case NODE_IDENTIFIER: {
            if (NULL == node->data.identifier.symbol->result.type) {
                node->data.identifier.symbol->result.type = type_basic(false, TYPE_BASIC_INT);
            }
            break;
        }
        case NODE_NUMBER: {
            node->data.number.result.type = type_basic(false, TYPE_BASIC_INT);
            break;
        }
        case NODE_STATEMENT_LIST: {
            type_ast_traversal(context, node->data.statement_list.init);
            type_ast_traversal(context, node->data.statement_list.statement);
            break;
        }
    }
}


/**************************
 * PRINT TYPE EXPRESSIONS *
 **************************/

static void type_print_basic(FILE *output, struct type *basic) {
    assert(TYPE_BASIC == basic->kind);

    if (basic->data.basic.is_unsigned) {
        fputs("unsigned", output);
    } else {
        fputs("  signed", output);
    }

    switch (basic->data.basic.datatype) {
        case TYPE_BASIC_INT:
            fputs("  int", output);
            break;
        case TYPE_BASIC_LONG:
            fputs(" long", output);
            break;
        default:
            assert(0);
    }
}

void type_print(FILE *output, struct type *kind) {
    assert(NULL != kind);

    switch (kind->kind) {
        case TYPE_BASIC:
            type_print_basic(output, kind);
            break;
        default:
            assert("only basic types supported for printing" && 0);
    }
}
