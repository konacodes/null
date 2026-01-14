#include "interp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Value utilities
static Value val_void(void) {
    return (Value){.kind = VAL_VOID};
}

static Value val_bool(bool b) {
    return (Value){.kind = VAL_BOOL, .bool_val = b};
}

static Value val_int(int64_t i) {
    return (Value){.kind = VAL_INT, .int_val = i};
}

static Value val_float(double f) {
    return (Value){.kind = VAL_FLOAT, .float_val = f};
}

static Value val_string(const char *s) {
    Value v = {.kind = VAL_STRING};
    v.string_val = strdup(s);
    return v;
}

static void val_free(Value *v) {
    if (v->kind == VAL_STRING && v->string_val) {
        free(v->string_val);
        v->string_val = NULL;
    } else if (v->kind == VAL_ARRAY) {
        for (int i = 0; i < v->array.count; i++) {
            val_free(&v->array.elements[i]);
        }
        free(v->array.elements);
    } else if (v->kind == VAL_STRUCT) {
        for (int i = 0; i < v->struct_val.field_count; i++) {
            free(v->struct_val.field_names[i]);
            val_free(&v->struct_val.field_values[i]);
        }
        free(v->struct_val.field_names);
        free(v->struct_val.field_values);
    }
}

static Value val_clone(Value v) {
    Value result = v;
    if (v.kind == VAL_STRING) {
        result.string_val = strdup(v.string_val);
    } else if (v.kind == VAL_ARRAY) {
        result.array.elements = malloc(sizeof(Value) * v.array.count);
        for (int i = 0; i < v.array.count; i++) {
            result.array.elements[i] = val_clone(v.array.elements[i]);
        }
    } else if (v.kind == VAL_STRUCT) {
        result.struct_val.field_names = malloc(sizeof(char*) * v.struct_val.field_count);
        result.struct_val.field_values = malloc(sizeof(Value) * v.struct_val.field_count);
        for (int i = 0; i < v.struct_val.field_count; i++) {
            result.struct_val.field_names[i] = strdup(v.struct_val.field_names[i]);
            result.struct_val.field_values[i] = val_clone(v.struct_val.field_values[i]);
        }
    }
    return result;
}

// Scope management
static InterpScope *scope_new(InterpScope *parent) {
    InterpScope *s = calloc(1, sizeof(InterpScope));
    s->parent = parent;
    s->capacity = 16;
    s->names = malloc(sizeof(char*) * s->capacity);
    s->values = malloc(sizeof(Value) * s->capacity);
    s->is_mut = malloc(sizeof(bool) * s->capacity);
    s->count = 0;
    return s;
}

static void scope_free(InterpScope *s) {
    for (int i = 0; i < s->count; i++) {
        free(s->names[i]);
        val_free(&s->values[i]);
    }
    free(s->names);
    free(s->values);
    free(s->is_mut);
    free(s);
}

static void scope_define(InterpScope *s, const char *name, Value val, bool is_mut) {
    if (s->count >= s->capacity) {
        s->capacity *= 2;
        s->names = realloc(s->names, sizeof(char*) * s->capacity);
        s->values = realloc(s->values, sizeof(Value) * s->capacity);
        s->is_mut = realloc(s->is_mut, sizeof(bool) * s->capacity);
    }
    s->names[s->count] = strdup(name);
    s->values[s->count] = val;
    s->is_mut[s->count] = is_mut;
    s->count++;
}

static Value *scope_lookup(InterpScope *s, const char *name) {
    for (InterpScope *scope = s; scope; scope = scope->parent) {
        for (int i = 0; i < scope->count; i++) {
            if (strcmp(scope->names[i], name) == 0) {
                return &scope->values[i];
            }
        }
    }
    return NULL;
}

static bool scope_assign(InterpScope *s, const char *name, Value val) {
    for (InterpScope *scope = s; scope; scope = scope->parent) {
        for (int i = 0; i < scope->count; i++) {
            if (strcmp(scope->names[i], name) == 0) {
                val_free(&scope->values[i]);
                scope->values[i] = val;
                return true;
            }
        }
    }
    return false;
}

// Error handling
static void interp_error(Interp *interp, const char *msg) {
    interp->had_error = true;
    interp->error_msg = strdup(msg);
    fprintf(stderr, "Runtime error: %s\n", msg);
}

// Forward declarations
static Value eval_expr(Interp *interp, ASTNode *node);
static void exec_stmt(Interp *interp, ASTNode *node);

// Find function by name
static InterpFunc *find_func(Interp *interp, const char *name) {
    for (int i = 0; i < interp->func_count; i++) {
        if (strcmp(interp->functions[i].name, name) == 0) {
            return &interp->functions[i];
        }
    }
    return NULL;
}

// Register function
static void register_func(Interp *interp, const char *name, ASTNode *node) {
    if (interp->func_count >= interp->func_capacity) {
        interp->func_capacity = interp->func_capacity ? interp->func_capacity * 2 : 16;
        interp->functions = realloc(interp->functions, sizeof(InterpFunc) * interp->func_capacity);
    }
    interp->functions[interp->func_count].name = strdup(name);
    interp->functions[interp->func_count].node = node;
    interp->func_count++;
}

// Call a function
static Value call_func(Interp *interp, const char *name, Value *args, int arg_count) {
    // Built-in functions
    if (strcmp(name, "puts") == 0 || strcmp(name, "io_print") == 0 || strcmp(name, "print") == 0) {
        if (arg_count > 0 && args[0].kind == VAL_STRING) {
            printf("%s\n", args[0].string_val);
        }
        return val_void();
    }
    if (strcmp(name, "print_raw") == 0 || strcmp(name, "printf") == 0) {
        if (arg_count > 0 && args[0].kind == VAL_STRING) {
            printf("%s", args[0].string_val);
        }
        return val_void();
    }
    if (strcmp(name, "print_int") == 0) {
        if (arg_count > 0 && args[0].kind == VAL_INT) {
            printf("%ld", args[0].int_val);
        }
        return val_void();
    }
    if (strcmp(name, "println") == 0) {
        printf("\n");
        return val_void();
    }
    if (strcmp(name, "putchar") == 0) {
        if (arg_count > 0 && args[0].kind == VAL_INT) {
            putchar((int)args[0].int_val);
        }
        return val_int(0);
    }
    if (strcmp(name, "getchar") == 0) {
        return val_int(getchar());
    }
    if (strcmp(name, "exit") == 0) {
        int code = (arg_count > 0 && args[0].kind == VAL_INT) ? (int)args[0].int_val : 0;
        exit(code);
    }

    // User-defined function
    InterpFunc *func = find_func(interp, name);
    if (!func) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Unknown function: %s", name);
        interp_error(interp, msg);
        return val_void();
    }

    // Create new scope for function
    InterpScope *prev_scope = interp->current_scope;
    interp->current_scope = scope_new(interp->global_scope);

    // Bind parameters
    ASTNode *fn = func->node;
    for (int i = 0; i < fn->fn_decl.param_count && i < arg_count; i++) {
        ASTNode *param = fn->fn_decl.params[i];
        scope_define(interp->current_scope, param->param.name, val_clone(args[i]), true);
    }

    // Execute function body
    interp->has_return = false;
    if (fn->fn_decl.body) {
        exec_stmt(interp, fn->fn_decl.body);
    }

    Value result = interp->has_return ? interp->return_value : val_void();
    interp->has_return = false;

    // Restore scope
    scope_free(interp->current_scope);
    interp->current_scope = prev_scope;

    return result;
}

// Evaluate expression
static Value eval_expr(Interp *interp, ASTNode *node) {
    if (!node || interp->had_error || interp->has_return) return val_void();

    switch (node->kind) {
        case NODE_LITERAL_INT:
            return val_int(node->int_val);

        case NODE_LITERAL_FLOAT:
            return val_float(node->float_val);

        case NODE_LITERAL_BOOL:
            return val_bool(node->bool_val);

        case NODE_LITERAL_STRING:
            return val_string(node->string_val);

        case NODE_IDENT: {
            Value *v = scope_lookup(interp->current_scope, node->ident);
            if (!v) {
                char msg[256];
                snprintf(msg, sizeof(msg), "Undefined variable: %s", node->ident);
                interp_error(interp, msg);
                return val_void();
            }
            return val_clone(*v);
        }

        case NODE_BINARY: {
            // Short-circuit for AND/OR
            if (node->binary.op == BIN_AND) {
                Value left = eval_expr(interp, node->binary.left);
                if (left.kind == VAL_BOOL && !left.bool_val) {
                    return val_bool(false);
                }
                Value right = eval_expr(interp, node->binary.right);
                return val_bool(left.bool_val && right.bool_val);
            }
            if (node->binary.op == BIN_OR) {
                Value left = eval_expr(interp, node->binary.left);
                if (left.kind == VAL_BOOL && left.bool_val) {
                    return val_bool(true);
                }
                Value right = eval_expr(interp, node->binary.right);
                return val_bool(left.bool_val || right.bool_val);
            }

            Value left = eval_expr(interp, node->binary.left);
            Value right = eval_expr(interp, node->binary.right);

            if (left.kind == VAL_INT && right.kind == VAL_INT) {
                switch (node->binary.op) {
                    case BIN_ADD: return val_int(left.int_val + right.int_val);
                    case BIN_SUB: return val_int(left.int_val - right.int_val);
                    case BIN_MUL: return val_int(left.int_val * right.int_val);
                    case BIN_DIV: return val_int(right.int_val ? left.int_val / right.int_val : 0);
                    case BIN_MOD: return val_int(right.int_val ? left.int_val % right.int_val : 0);
                    case BIN_EQ: return val_bool(left.int_val == right.int_val);
                    case BIN_NE: return val_bool(left.int_val != right.int_val);
                    case BIN_LT: return val_bool(left.int_val < right.int_val);
                    case BIN_LE: return val_bool(left.int_val <= right.int_val);
                    case BIN_GT: return val_bool(left.int_val > right.int_val);
                    case BIN_GE: return val_bool(left.int_val >= right.int_val);
                    case BIN_BAND: return val_int(left.int_val & right.int_val);
                    case BIN_BOR: return val_int(left.int_val | right.int_val);
                    case BIN_BXOR: return val_int(left.int_val ^ right.int_val);
                    case BIN_LSHIFT: return val_int(left.int_val << right.int_val);
                    case BIN_RSHIFT: return val_int(left.int_val >> right.int_val);
                    default: break;
                }
            }
            if (left.kind == VAL_FLOAT || right.kind == VAL_FLOAT) {
                double l = left.kind == VAL_FLOAT ? left.float_val : (double)left.int_val;
                double r = right.kind == VAL_FLOAT ? right.float_val : (double)right.int_val;
                switch (node->binary.op) {
                    case BIN_ADD: return val_float(l + r);
                    case BIN_SUB: return val_float(l - r);
                    case BIN_MUL: return val_float(l * r);
                    case BIN_DIV: return val_float(r != 0 ? l / r : 0);
                    case BIN_EQ: return val_bool(l == r);
                    case BIN_NE: return val_bool(l != r);
                    case BIN_LT: return val_bool(l < r);
                    case BIN_LE: return val_bool(l <= r);
                    case BIN_GT: return val_bool(l > r);
                    case BIN_GE: return val_bool(l >= r);
                    default: break;
                }
            }
            if (left.kind == VAL_BOOL && right.kind == VAL_BOOL) {
                switch (node->binary.op) {
                    case BIN_EQ: return val_bool(left.bool_val == right.bool_val);
                    case BIN_NE: return val_bool(left.bool_val != right.bool_val);
                    default: break;
                }
            }
            return val_void();
        }

        case NODE_UNARY: {
            Value operand = eval_expr(interp, node->unary.operand);
            switch (node->unary.op) {
                case UN_NEG:
                    if (operand.kind == VAL_INT) return val_int(-operand.int_val);
                    if (operand.kind == VAL_FLOAT) return val_float(-operand.float_val);
                    break;
                case UN_NOT:
                    if (operand.kind == VAL_BOOL) return val_bool(!operand.bool_val);
                    break;
                case UN_BNOT:
                    if (operand.kind == VAL_INT) return val_int(~operand.int_val);
                    break;
                default:
                    break;
            }
            return val_void();
        }

        case NODE_CALL: {
            // Get function name
            const char *func_name = NULL;
            if (node->call.callee->kind == NODE_IDENT) {
                func_name = node->call.callee->ident;
            } else {
                interp_error(interp, "Invalid function call");
                return val_void();
            }

            // Evaluate arguments
            Value *args = malloc(sizeof(Value) * node->call.arg_count);
            for (int i = 0; i < node->call.arg_count; i++) {
                args[i] = eval_expr(interp, node->call.args[i]);
            }

            Value result = call_func(interp, func_name, args, node->call.arg_count);

            for (int i = 0; i < node->call.arg_count; i++) {
                val_free(&args[i]);
            }
            free(args);

            return result;
        }

        case NODE_INDEX: {
            Value arr = eval_expr(interp, node->index.object);
            Value idx = eval_expr(interp, node->index.index);
            if (arr.kind == VAL_ARRAY && idx.kind == VAL_INT) {
                int i = (int)idx.int_val;
                if (i >= 0 && i < arr.array.count) {
                    Value result = val_clone(arr.array.elements[i]);
                    val_free(&arr);
                    return result;
                }
            }
            val_free(&arr);
            interp_error(interp, "Invalid array index");
            return val_void();
        }

        case NODE_MEMBER: {
            Value obj = eval_expr(interp, node->member.object);
            if (obj.kind == VAL_STRUCT) {
                for (int i = 0; i < obj.struct_val.field_count; i++) {
                    if (strcmp(obj.struct_val.field_names[i], node->member.member) == 0) {
                        Value result = val_clone(obj.struct_val.field_values[i]);
                        val_free(&obj);
                        return result;
                    }
                }
            }
            val_free(&obj);
            interp_error(interp, "Invalid member access");
            return val_void();
        }

        case NODE_ARRAY_INIT: {
            Value arr = {.kind = VAL_ARRAY};
            arr.array.count = node->array_init.elem_count;
            arr.array.elements = malloc(sizeof(Value) * arr.array.count);
            for (int i = 0; i < arr.array.count; i++) {
                arr.array.elements[i] = eval_expr(interp, node->array_init.elements[i]);
            }
            return arr;
        }

        case NODE_STRUCT_INIT: {
            Value s = {.kind = VAL_STRUCT};
            s.struct_val.field_count = node->struct_init.field_count;
            s.struct_val.field_names = malloc(sizeof(char*) * s.struct_val.field_count);
            s.struct_val.field_values = malloc(sizeof(Value) * s.struct_val.field_count);
            for (int i = 0; i < s.struct_val.field_count; i++) {
                s.struct_val.field_names[i] = strdup(node->struct_init.field_names[i]);
                s.struct_val.field_values[i] = eval_expr(interp, node->struct_init.field_values[i]);
            }
            return s;
        }

        case NODE_ASSIGN: {
            Value val = eval_expr(interp, node->assign.value);
            if (node->assign.target->kind == NODE_IDENT) {
                scope_assign(interp->current_scope, node->assign.target->ident, val);
                return val_clone(val);
            } else if (node->assign.target->kind == NODE_INDEX) {
                // Array element assignment
                Value *arr = scope_lookup(interp->current_scope, node->assign.target->index.object->ident);
                Value idx = eval_expr(interp, node->assign.target->index.index);
                if (arr && arr->kind == VAL_ARRAY && idx.kind == VAL_INT) {
                    int i = (int)idx.int_val;
                    if (i >= 0 && i < arr->array.count) {
                        val_free(&arr->array.elements[i]);
                        arr->array.elements[i] = val;
                        return val_clone(val);
                    }
                }
            } else if (node->assign.target->kind == NODE_MEMBER) {
                // Struct member assignment
                Value *obj = scope_lookup(interp->current_scope, node->assign.target->member.object->ident);
                if (obj && obj->kind == VAL_STRUCT) {
                    for (int i = 0; i < obj->struct_val.field_count; i++) {
                        if (strcmp(obj->struct_val.field_names[i], node->assign.target->member.member) == 0) {
                            val_free(&obj->struct_val.field_values[i]);
                            obj->struct_val.field_values[i] = val;
                            return val_clone(val);
                        }
                    }
                }
            }
            return val;
        }

        default:
            return val_void();
    }
}

// Execute statement
static void exec_stmt(Interp *interp, ASTNode *node) {
    if (!node || interp->had_error || interp->has_return ||
        interp->has_break || interp->has_continue) return;

    switch (node->kind) {
        case NODE_BLOCK:
            for (int i = 0; i < node->block.stmt_count &&
                 !interp->has_return && !interp->has_break && !interp->has_continue; i++) {
                exec_stmt(interp, node->block.stmts[i]);
            }
            break;

        case NODE_VAR_DECL: {
            Value val = node->var_decl.init ? eval_expr(interp, node->var_decl.init) : val_void();
            scope_define(interp->current_scope, node->var_decl.name, val, node->var_decl.is_mut);
            break;
        }

        case NODE_RETURN:
            if (node->ret.value) {
                interp->return_value = eval_expr(interp, node->ret.value);
            } else {
                interp->return_value = val_void();
            }
            interp->has_return = true;
            break;

        case NODE_BREAK:
            if (interp->loop_depth == 0) {
                interp_error(interp, "'break' outside of loop");
                return;
            }
            interp->has_break = true;
            break;

        case NODE_CONTINUE:
            if (interp->loop_depth == 0) {
                interp_error(interp, "'continue' outside of loop");
                return;
            }
            interp->has_continue = true;
            break;

        case NODE_IF: {
            Value cond = eval_expr(interp, node->if_stmt.cond);
            if (cond.kind == VAL_BOOL && cond.bool_val) {
                exec_stmt(interp, node->if_stmt.then_block);
            } else {
                bool handled = false;
                for (int i = 0; i < node->if_stmt.elif_count && !handled; i++) {
                    Value elif_cond = eval_expr(interp, node->if_stmt.elif_conds[i]);
                    if (elif_cond.kind == VAL_BOOL && elif_cond.bool_val) {
                        exec_stmt(interp, node->if_stmt.elif_blocks[i]);
                        handled = true;
                    }
                }
                if (!handled && node->if_stmt.else_block) {
                    exec_stmt(interp, node->if_stmt.else_block);
                }
            }
            break;
        }

        case NODE_WHILE: {
            interp->loop_depth++;
            while (!interp->has_return && !interp->had_error && !interp->has_break) {
                Value cond = eval_expr(interp, node->while_stmt.cond);
                if (cond.kind != VAL_BOOL || !cond.bool_val) break;
                exec_stmt(interp, node->while_stmt.body);
                interp->has_continue = false;  // Reset continue for next iteration
            }
            interp->has_break = false;  // Reset break after loop
            interp->loop_depth--;
            break;
        }

        case NODE_FOR: {
            Value start = eval_expr(interp, node->for_stmt.start);
            Value end = eval_expr(interp, node->for_stmt.end);
            if (start.kind == VAL_INT && end.kind == VAL_INT) {
                InterpScope *loop_scope = scope_new(interp->current_scope);
                InterpScope *prev = interp->current_scope;
                interp->current_scope = loop_scope;

                scope_define(loop_scope, node->for_stmt.var_name, start, true);
                Value *iter = scope_lookup(loop_scope, node->for_stmt.var_name);

                interp->loop_depth++;
                while (iter->int_val < end.int_val && !interp->has_return &&
                       !interp->had_error && !interp->has_break) {
                    exec_stmt(interp, node->for_stmt.body);
                    interp->has_continue = false;  // Reset continue for next iteration
                    iter->int_val++;
                }
                interp->has_break = false;  // Reset break after loop
                interp->loop_depth--;

                interp->current_scope = prev;
                scope_free(loop_scope);
            }
            break;
        }

        case NODE_EXPR_STMT: {
            Value v = eval_expr(interp, node->expr_stmt.expr);
            val_free(&v);
            break;
        }

        case NODE_ASSIGN: {
            Value v = eval_expr(interp, node);
            val_free(&v);
            break;
        }

        default:
            break;
    }
}

// Initialize interpreter
void interp_init(Interp *interp) {
    memset(interp, 0, sizeof(Interp));
    interp->global_scope = scope_new(NULL);
    interp->current_scope = interp->global_scope;
}

// Free interpreter
void interp_free(Interp *interp) {
    if (interp->global_scope) {
        scope_free(interp->global_scope);
    }
    for (int i = 0; i < interp->func_count; i++) {
        free(interp->functions[i].name);
    }
    free(interp->functions);
    if (interp->error_msg) free(interp->error_msg);
}

// Run program
int interp_run(Interp *interp, ASTNode *ast) {
    if (!ast || ast->kind != NODE_PROGRAM) {
        interp_error(interp, "Invalid program");
        return 1;
    }

    // First pass: register all functions and struct definitions
    for (int i = 0; i < ast->program.decl_count; i++) {
        ASTNode *decl = ast->program.decls[i];
        if (decl->kind == NODE_FN_DECL && !decl->fn_decl.is_extern) {
            register_func(interp, decl->fn_decl.name, decl);
        }
    }

    // Look for main function, or __repl_main__ for REPL mode
    InterpFunc *main_fn = find_func(interp, "main");
    const char *entry_point = "main";
    if (!main_fn) {
        main_fn = find_func(interp, "__repl_main__");
        entry_point = "__repl_main__";
        if (!main_fn) {
            interp_error(interp, "No main function found");
            return 1;
        }
    }

    Value result = call_func(interp, entry_point, NULL, 0);
    int exit_code = (result.kind == VAL_INT) ? (int)result.int_val : 0;
    val_free(&result);

    return interp->had_error ? 1 : exit_code;
}
