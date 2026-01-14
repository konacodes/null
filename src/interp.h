#ifndef NULL_INTERP_H
#define NULL_INTERP_H

#include "parser.h"
#include <stdbool.h>

// Value types for the interpreter
typedef enum {
    VAL_VOID,
    VAL_BOOL,
    VAL_INT,
    VAL_FLOAT,
    VAL_STRING,
    VAL_ARRAY,
    VAL_STRUCT,
} ValueKind;

typedef struct Value Value;

struct Value {
    ValueKind kind;
    union {
        bool bool_val;
        int64_t int_val;
        double float_val;
        char *string_val;
        struct {
            Value *elements;
            int count;
        } array;
        struct {
            char **field_names;
            Value *field_values;
            int field_count;
        } struct_val;
    };
};

// Interpreter scope for variables
typedef struct InterpScope {
    struct InterpScope *parent;
    char **names;
    Value *values;
    bool *is_mut;
    int count;
    int capacity;
} InterpScope;

// Function definition for interpreter
typedef struct {
    char *name;
    ASTNode *node;  // The function AST node
} InterpFunc;

// Interpreter state
typedef struct {
    InterpScope *global_scope;
    InterpScope *current_scope;
    InterpFunc *functions;
    int func_count;
    int func_capacity;
    Value return_value;
    bool has_return;
    bool has_break;     // break statement executed
    bool has_continue;  // continue statement executed
    int loop_depth;     // current nesting level of loops
    bool had_error;
    char *error_msg;
} Interp;

// Initialize interpreter
void interp_init(Interp *interp);

// Free interpreter
void interp_free(Interp *interp);

// Run program
int interp_run(Interp *interp, ASTNode *ast);

#endif // NULL_INTERP_H
