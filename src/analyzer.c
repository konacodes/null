#include "analyzer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations
static void analyze_node(Analyzer *a, ASTNode *node);
static void analyze_fn_decl(Analyzer *a, ASTNode *node);
static void analyze_struct_decl(Analyzer *a, ASTNode *node);
static void analyze_var_decl(Analyzer *a, ASTNode *node);
static void analyze_block(Analyzer *a, ASTNode *node);
static void analyze_stmt(Analyzer *a, ASTNode *node);
static void analyze_expr(Analyzer *a, ASTNode *node);
static Type *infer_type(Analyzer *a, ASTNode *node);

static void error(Analyzer *a, ASTNode *node, const char *msg) {
    if (a->had_error) return;
    a->had_error = true;
    fprintf(stderr, "[%d:%d] Error: %s\n", node->line, node->column, msg);
}

static void error_at(Analyzer *a, int line, int col, const char *msg) {
    if (a->had_error) return;
    a->had_error = true;
    fprintf(stderr, "[%d:%d] Error: %s\n", line, col, msg);
}

// Scope management
Scope *scope_new(Scope *parent) {
    Scope *s = calloc(1, sizeof(Scope));
    s->parent = parent;
    s->symbols = NULL;
    return s;
}

void scope_free(Scope *s) {
    if (!s) return;
    Symbol *sym = s->symbols;
    while (sym) {
        Symbol *next = sym->next;
        symbol_free(sym);
        sym = next;
    }
    free(s);
}

Symbol *scope_lookup(Scope *s, const char *name) {
    while (s) {
        Symbol *sym = scope_lookup_local(s, name);
        if (sym) return sym;
        s = s->parent;
    }
    return NULL;
}

Symbol *scope_lookup_local(Scope *s, const char *name) {
    for (Symbol *sym = s->symbols; sym; sym = sym->next) {
        if (strcmp(sym->name, name) == 0) {
            return sym;
        }
    }
    return NULL;
}

void scope_define(Scope *s, Symbol *sym) {
    sym->next = s->symbols;
    s->symbols = sym;
}

// Symbol management
Symbol *symbol_new(const char *name, SymbolKind kind, Type *type) {
    Symbol *sym = calloc(1, sizeof(Symbol));
    sym->name = strdup(name);
    sym->kind = kind;
    sym->type = type;
    sym->is_mut = false;
    sym->is_extern = false;
    sym->decl = NULL;
    sym->next = NULL;
    return sym;
}

void symbol_free(Symbol *sym) {
    if (!sym) return;
    free(sym->name);
    // Don't free type - owned by AST
    free(sym);
}

// Analyzer
void analyzer_init(Analyzer *a) {
    a->global_scope = scope_new(NULL);
    a->current_scope = a->global_scope;
    a->current_fn_ret_type = NULL;
    a->had_error = false;
    a->error_msg = NULL;
}

void analyzer_free(Analyzer *a) {
    // Free all scopes starting from global
    Scope *s = a->global_scope;
    while (s) {
        Scope *next = s->parent;  // This will be NULL for global
        scope_free(s);
        s = next;
    }
}

bool analyzer_analyze(Analyzer *a, ASTNode *ast) {
    if (!ast || ast->kind != NODE_PROGRAM) {
        return false;
    }

    // First pass: collect all top-level declarations
    for (int i = 0; i < ast->program.decl_count; i++) {
        ASTNode *decl = ast->program.decls[i];
        switch (decl->kind) {
            case NODE_FN_DECL: {
                Symbol *sym = symbol_new(decl->fn_decl.name, SYM_FN, NULL);
                sym->decl = decl;
                sym->is_extern = decl->fn_decl.is_extern;

                // Build function type
                Type *fn_type = type_new(TYPE_FN);
                fn_type->fn.ret_type = type_clone(decl->fn_decl.ret_type);
                fn_type->fn.param_count = decl->fn_decl.param_count;
                fn_type->fn.param_types = malloc(sizeof(Type*) * decl->fn_decl.param_count);
                for (int j = 0; j < decl->fn_decl.param_count; j++) {
                    fn_type->fn.param_types[j] = type_clone(decl->fn_decl.params[j]->param.param_type);
                }
                sym->type = fn_type;

                if (scope_lookup_local(a->global_scope, decl->fn_decl.name)) {
                    error(a, decl, "Duplicate function declaration.");
                } else {
                    scope_define(a->global_scope, sym);
                }
                break;
            }
            case NODE_STRUCT_DECL: {
                Symbol *sym = symbol_new(decl->struct_decl.name, SYM_STRUCT, NULL);
                sym->decl = decl;

                // Build struct type
                Type *struct_type = type_new(TYPE_STRUCT);
                struct_type->struct_t.name = strdup(decl->struct_decl.name);
                struct_type->struct_t.field_count = decl->struct_decl.field_count;
                struct_type->struct_t.field_names = malloc(sizeof(char*) * decl->struct_decl.field_count);
                struct_type->struct_t.field_types = malloc(sizeof(Type*) * decl->struct_decl.field_count);
                for (int j = 0; j < decl->struct_decl.field_count; j++) {
                    struct_type->struct_t.field_names[j] = strdup(decl->struct_decl.field_names[j]);
                    struct_type->struct_t.field_types[j] = type_clone(decl->struct_decl.field_types[j]);
                }
                sym->type = struct_type;

                if (scope_lookup_local(a->global_scope, decl->struct_decl.name)) {
                    error(a, decl, "Duplicate struct declaration.");
                } else {
                    scope_define(a->global_scope, sym);
                }
                break;
            }
            case NODE_EXTERN: {
                for (int j = 0; j < decl->extern_decl.fn_count; j++) {
                    ASTNode *fn = decl->extern_decl.fn_decls[j];
                    Symbol *sym = symbol_new(fn->fn_decl.name, SYM_FN, NULL);
                    sym->decl = fn;
                    sym->is_extern = true;

                    Type *fn_type = type_new(TYPE_FN);
                    fn_type->fn.ret_type = type_clone(fn->fn_decl.ret_type);
                    fn_type->fn.param_count = fn->fn_decl.param_count;
                    fn_type->fn.param_types = malloc(sizeof(Type*) * fn->fn_decl.param_count);
                    for (int k = 0; k < fn->fn_decl.param_count; k++) {
                        fn_type->fn.param_types[k] = type_clone(fn->fn_decl.params[k]->param.param_type);
                    }
                    sym->type = fn_type;

                    if (scope_lookup_local(a->global_scope, fn->fn_decl.name)) {
                        error(a, fn, "Duplicate function declaration.");
                    } else {
                        scope_define(a->global_scope, sym);
                    }
                }
                break;
            }
            default:
                break;
        }
    }

    // Second pass: analyze bodies
    for (int i = 0; i < ast->program.decl_count; i++) {
        analyze_node(a, ast->program.decls[i]);
    }

    return !a->had_error;
}

static void analyze_node(Analyzer *a, ASTNode *node) {
    if (!node) return;

    switch (node->kind) {
        case NODE_FN_DECL:
            analyze_fn_decl(a, node);
            break;
        case NODE_STRUCT_DECL:
            analyze_struct_decl(a, node);
            break;
        case NODE_VAR_DECL:
            analyze_var_decl(a, node);
            break;
        case NODE_BLOCK:
            analyze_block(a, node);
            break;
        case NODE_USE:
            // Module imports handled separately
            break;
        case NODE_EXTERN:
            // Already handled in first pass
            break;
        default:
            analyze_stmt(a, node);
            break;
    }
}

static void analyze_fn_decl(Analyzer *a, ASTNode *node) {
    if (node->fn_decl.is_extern && !node->fn_decl.body) {
        return; // extern functions don't have bodies
    }

    // Create function scope
    Scope *fn_scope = scope_new(a->current_scope);
    a->current_scope = fn_scope;
    a->current_fn_ret_type = node->fn_decl.ret_type;

    // Add parameters to scope
    for (int i = 0; i < node->fn_decl.param_count; i++) {
        ASTNode *param = node->fn_decl.params[i];
        Symbol *sym = symbol_new(param->param.name, SYM_PARAM, param->param.param_type);
        sym->decl = param;
        scope_define(fn_scope, sym);
    }

    // Analyze body
    if (node->fn_decl.body) {
        analyze_block(a, node->fn_decl.body);
    }

    a->current_scope = fn_scope->parent;
    a->current_fn_ret_type = NULL;
    scope_free(fn_scope);
}

static void analyze_struct_decl(Analyzer *a, ASTNode *node) {
    // Struct fields are validated during type creation
    (void)a;
    (void)node;
}

static void analyze_var_decl(Analyzer *a, ASTNode *node) {
    // Check for duplicate in current scope
    if (scope_lookup_local(a->current_scope, node->var_decl.name)) {
        error(a, node, "Variable already declared in this scope.");
        return;
    }

    // Analyze initializer
    if (node->var_decl.init) {
        analyze_expr(a, node->var_decl.init);
    }

    // Infer type if not specified
    Type *var_type = node->var_decl.var_type;
    if (!var_type && node->var_decl.init) {
        var_type = infer_type(a, node->var_decl.init);
        node->var_decl.var_type = var_type;
    }

    if (!var_type) {
        error(a, node, "Cannot infer type for variable.");
        var_type = type_new(TYPE_UNKNOWN);
        node->var_decl.var_type = var_type;
    }

    // Create symbol
    Symbol *sym = symbol_new(node->var_decl.name, SYM_VAR, var_type);
    sym->is_mut = node->var_decl.is_mut;
    sym->decl = node;
    scope_define(a->current_scope, sym);
}

static void analyze_block(Analyzer *a, ASTNode *node) {
    Scope *block_scope = scope_new(a->current_scope);
    a->current_scope = block_scope;

    for (int i = 0; i < node->block.stmt_count; i++) {
        analyze_stmt(a, node->block.stmts[i]);
    }

    a->current_scope = block_scope->parent;
    scope_free(block_scope);
}

static void analyze_stmt(Analyzer *a, ASTNode *node) {
    if (!node) return;

    switch (node->kind) {
        case NODE_VAR_DECL:
            analyze_var_decl(a, node);
            break;
        case NODE_RETURN:
            if (node->ret.value) {
                analyze_expr(a, node->ret.value);
            }
            break;
        case NODE_IF:
            analyze_expr(a, node->if_stmt.cond);
            analyze_block(a, node->if_stmt.then_block);
            for (int i = 0; i < node->if_stmt.elif_count; i++) {
                analyze_expr(a, node->if_stmt.elif_conds[i]);
                analyze_block(a, node->if_stmt.elif_blocks[i]);
            }
            if (node->if_stmt.else_block) {
                analyze_block(a, node->if_stmt.else_block);
            }
            break;
        case NODE_WHILE:
            analyze_expr(a, node->while_stmt.cond);
            analyze_block(a, node->while_stmt.body);
            break;
        case NODE_FOR: {
            // Create loop scope with iterator variable
            Scope *loop_scope = scope_new(a->current_scope);
            a->current_scope = loop_scope;

            analyze_expr(a, node->for_stmt.start);
            analyze_expr(a, node->for_stmt.end);

            // Add iterator variable
            Type *iter_type = infer_type(a, node->for_stmt.start);
            if (!iter_type) iter_type = type_new(TYPE_I64);
            Symbol *iter_sym = symbol_new(node->for_stmt.var_name, SYM_VAR, iter_type);
            scope_define(loop_scope, iter_sym);

            analyze_block(a, node->for_stmt.body);

            a->current_scope = loop_scope->parent;
            scope_free(loop_scope);
            break;
        }
        case NODE_ASSIGN:
            analyze_expr(a, node->assign.target);
            analyze_expr(a, node->assign.value);

            // Check mutability
            if (node->assign.target->kind == NODE_IDENT) {
                Symbol *sym = scope_lookup(a->current_scope, node->assign.target->ident);
                if (sym && !sym->is_mut && sym->kind == SYM_VAR) {
                    error(a, node, "Cannot assign to immutable variable.");
                }
            }
            break;
        case NODE_EXPR_STMT:
            analyze_expr(a, node->expr_stmt.expr);
            break;
        default:
            break;
    }
}

static void analyze_expr(Analyzer *a, ASTNode *node) {
    if (!node) return;

    switch (node->kind) {
        case NODE_BINARY:
            analyze_expr(a, node->binary.left);
            analyze_expr(a, node->binary.right);
            break;
        case NODE_UNARY:
            analyze_expr(a, node->unary.operand);
            break;
        case NODE_CALL:
            analyze_expr(a, node->call.callee);
            for (int i = 0; i < node->call.arg_count; i++) {
                analyze_expr(a, node->call.args[i]);
            }

            // Verify function exists
            if (node->call.callee->kind == NODE_IDENT) {
                Symbol *sym = scope_lookup(a->current_scope, node->call.callee->ident);
                if (!sym) {
                    char msg[256];
                    snprintf(msg, sizeof(msg), "Unknown function: %s", node->call.callee->ident);
                    error(a, node, msg);
                } else if (sym->kind != SYM_FN) {
                    error(a, node, "Cannot call non-function.");
                }
            } else if (node->call.callee->kind == NODE_MEMBER) {
                // Module.function call - handled separately
            }
            break;
        case NODE_MEMBER:
            analyze_expr(a, node->member.object);
            break;
        case NODE_INDEX:
            analyze_expr(a, node->index.object);
            analyze_expr(a, node->index.index);
            break;
        case NODE_IDENT: {
            Symbol *sym = scope_lookup(a->current_scope, node->ident);
            if (!sym) {
                // Could be a module name - defer check
                // char msg[256];
                // snprintf(msg, sizeof(msg), "Unknown identifier: %s", node->ident);
                // error(a, node, msg);
            }
            break;
        }
        case NODE_STRUCT_INIT: {
            Symbol *sym = scope_lookup(a->current_scope, node->struct_init.struct_name);
            if (!sym || sym->kind != SYM_STRUCT) {
                char msg[256];
                snprintf(msg, sizeof(msg), "Unknown struct: %s", node->struct_init.struct_name);
                error(a, node, msg);
            }
            for (int i = 0; i < node->struct_init.field_count; i++) {
                analyze_expr(a, node->struct_init.field_values[i]);
            }
            break;
        }
        case NODE_ARRAY_INIT:
            for (int i = 0; i < node->array_init.elem_count; i++) {
                analyze_expr(a, node->array_init.elements[i]);
            }
            break;
        default:
            break;
    }
}

static Type *infer_type(Analyzer *a, ASTNode *node) {
    if (!node) return NULL;

    switch (node->kind) {
        case NODE_LITERAL_INT:
            return type_new(TYPE_I64);
        case NODE_LITERAL_FLOAT:
            return type_new(TYPE_F64);
        case NODE_LITERAL_STRING:
            return type_new(TYPE_SLICE); // [u8]
        case NODE_LITERAL_BOOL:
            return type_new(TYPE_BOOL);
        case NODE_IDENT: {
            Symbol *sym = scope_lookup(a->current_scope, node->ident);
            if (sym && sym->type) {
                return type_clone(sym->type);
            }
            return NULL;
        }
        case NODE_BINARY:
            // For arithmetic, return left operand's type
            return infer_type(a, node->binary.left);
        case NODE_UNARY:
            return infer_type(a, node->unary.operand);
        case NODE_CALL: {
            if (node->call.callee->kind == NODE_IDENT) {
                Symbol *sym = scope_lookup(a->current_scope, node->call.callee->ident);
                if (sym && sym->type && sym->type->kind == TYPE_FN) {
                    return type_clone(sym->type->fn.ret_type);
                }
            }
            return NULL;
        }
        case NODE_STRUCT_INIT: {
            Type *t = type_new(TYPE_STRUCT);
            t->struct_t.name = strdup(node->struct_init.struct_name);
            return t;
        }
        default:
            return NULL;
    }
}
