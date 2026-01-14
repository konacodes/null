#ifndef NULL_PARSER_H
#define NULL_PARSER_H

#include "lexer.h"
#include <stdbool.h>

// Forward declarations
typedef struct ASTNode ASTNode;
typedef struct Type Type;

// Type kinds
typedef enum {
    TYPE_VOID,
    TYPE_BOOL,
    TYPE_I8,
    TYPE_I16,
    TYPE_I32,
    TYPE_I64,
    TYPE_U8,
    TYPE_U16,
    TYPE_U32,
    TYPE_U64,
    TYPE_F32,
    TYPE_F64,
    TYPE_PTR,
    TYPE_ARRAY,
    TYPE_SLICE,
    TYPE_STRUCT,
    TYPE_ENUM,
    TYPE_FN,
    TYPE_UNKNOWN,
} TypeKind;

struct Type {
    TypeKind kind;
    int line;       // Source position for error reporting
    int column;
    union {
        Type *ptr_to;           // for TYPE_PTR
        struct {                // for TYPE_ARRAY
            Type *elem_type;
            int size;           // -1 for slices
        } array;
        struct {                // for TYPE_STRUCT
            char *name;
            int field_count;
            char **field_names;
            Type **field_types;
        } struct_t;
        struct {                // for TYPE_ENUM
            char *name;
            int variant_count;
            char **variant_names;
            int64_t *variant_values;
        } enum_t;
        struct {                // for TYPE_FN
            Type *ret_type;
            int param_count;
            Type **param_types;
        } fn;
    };
};

// AST Node kinds
typedef enum {
    // Program
    NODE_PROGRAM,

    // Declarations
    NODE_FN_DECL,
    NODE_STRUCT_DECL,
    NODE_ENUM_DECL,
    NODE_VAR_DECL,
    NODE_PARAM,

    // Statements
    NODE_BLOCK,
    NODE_RETURN,
    NODE_BREAK,
    NODE_CONTINUE,
    NODE_IF,
    NODE_WHILE,
    NODE_FOR,
    NODE_EXPR_STMT,
    NODE_ASSIGN,

    // Expressions
    NODE_BINARY,
    NODE_UNARY,
    NODE_CALL,
    NODE_MEMBER,
    NODE_INDEX,
    NODE_LITERAL_INT,
    NODE_LITERAL_FLOAT,
    NODE_LITERAL_STRING,
    NODE_LITERAL_BOOL,
    NODE_IDENT,
    NODE_STRUCT_INIT,
    NODE_ARRAY_INIT,
    NODE_ENUM_VARIANT,

    // Directives
    NODE_USE,
    NODE_EXTERN,
} NodeKind;

// Binary operators
typedef enum {
    BIN_ADD,
    BIN_SUB,
    BIN_MUL,
    BIN_DIV,
    BIN_MOD,
    BIN_EQ,
    BIN_NE,
    BIN_LT,
    BIN_LE,
    BIN_GT,
    BIN_GE,
    BIN_AND,
    BIN_OR,
    BIN_BAND,
    BIN_BOR,
    BIN_BXOR,
    BIN_LSHIFT,
    BIN_RSHIFT,
    BIN_PIPE,   // |>
} BinaryOp;

// Unary operators
typedef enum {
    UN_NEG,     // -
    UN_NOT,     // not
    UN_BNOT,    // ~
    UN_ADDR,    // &
    UN_DEREF,   // *
} UnaryOp;

// AST Node structure
struct ASTNode {
    NodeKind kind;
    int line;
    int column;
    Type *type;  // resolved type (filled during analysis)

    union {
        // NODE_PROGRAM
        struct {
            ASTNode **decls;
            int decl_count;
            int decl_capacity;
        } program;

        // NODE_FN_DECL
        struct {
            char *name;
            ASTNode **params;
            int param_count;
            int param_capacity;
            Type *ret_type;
            ASTNode *body;
            bool is_extern;
        } fn_decl;

        // NODE_STRUCT_DECL
        struct {
            char *name;
            char **field_names;
            Type **field_types;
            int field_count;
        } struct_decl;

        // NODE_ENUM_DECL
        struct {
            char *name;
            char **variant_names;
            int64_t *variant_values;
            int variant_count;
        } enum_decl;

        // NODE_VAR_DECL
        struct {
            char *name;
            Type *var_type;
            ASTNode *init;
            bool is_mut;
            bool is_const;  // compile-time constant
        } var_decl;

        // NODE_PARAM
        struct {
            char *name;
            Type *param_type;
        } param;

        // NODE_BLOCK
        struct {
            ASTNode **stmts;
            int stmt_count;
            int stmt_capacity;
        } block;

        // NODE_RETURN
        struct {
            ASTNode *value;
        } ret;

        // NODE_IF
        struct {
            ASTNode *cond;
            ASTNode *then_block;
            ASTNode **elif_conds;
            ASTNode **elif_blocks;
            int elif_count;
            ASTNode *else_block;
        } if_stmt;

        // NODE_WHILE
        struct {
            ASTNode *cond;
            ASTNode *body;
        } while_stmt;

        // NODE_FOR
        struct {
            char *var_name;
            ASTNode *start;
            ASTNode *end;
            ASTNode *body;
        } for_stmt;

        // NODE_ASSIGN
        struct {
            ASTNode *target;
            ASTNode *value;
        } assign;

        // NODE_BINARY
        struct {
            BinaryOp op;
            ASTNode *left;
            ASTNode *right;
        } binary;

        // NODE_UNARY
        struct {
            UnaryOp op;
            ASTNode *operand;
        } unary;

        // NODE_CALL
        struct {
            ASTNode *callee;
            ASTNode **args;
            int arg_count;
        } call;

        // NODE_MEMBER
        struct {
            ASTNode *object;
            char *member;
        } member;

        // NODE_INDEX
        struct {
            ASTNode *object;
            ASTNode *index;
        } index;

        // NODE_LITERAL_INT
        int64_t int_val;

        // NODE_LITERAL_FLOAT
        double float_val;

        // NODE_LITERAL_STRING
        char *string_val;

        // NODE_LITERAL_BOOL
        bool bool_val;

        // NODE_IDENT
        char *ident;

        // NODE_STRUCT_INIT
        struct {
            char *struct_name;
            char **field_names;
            ASTNode **field_values;
            int field_count;
        } struct_init;

        // NODE_ARRAY_INIT
        struct {
            ASTNode **elements;
            int elem_count;
        } array_init;

        // NODE_ENUM_VARIANT
        struct {
            char *enum_name;
            char *variant_name;
        } enum_variant;

        // NODE_USE
        struct {
            char *path;
            char *alias;
        } use;

        // NODE_EXTERN
        struct {
            char *abi;
            ASTNode **fn_decls;
            int fn_count;
        } extern_decl;

        // NODE_EXPR_STMT
        struct {
            ASTNode *expr;
        } expr_stmt;
    };
};

// Parser state
typedef struct {
    Lexer *lexer;
    Token current;
    Token previous;
    bool had_error;
    bool panic_mode;
    char *error_msg;
} Parser;

// Initialize parser
void parser_init(Parser *parser, Lexer *lexer);

// Parse source into AST
ASTNode *parser_parse(Parser *parser);

// Free AST
void ast_free(ASTNode *node);

// Type utilities
Type *type_new(TypeKind kind);
Type *type_new_at(TypeKind kind, int line, int column);
Type *type_clone(Type *t);
void type_free(Type *t);
char *type_to_string(Type *t);
bool type_equals(Type *a, Type *b);

// AST utilities
ASTNode *ast_new(NodeKind kind, int line, int column);
void ast_print(ASTNode *node, int indent);

#endif // NULL_PARSER_H
