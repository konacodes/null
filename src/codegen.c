#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations
static LLVMValueRef codegen_expr(Codegen *cg, ASTNode *node);
static void codegen_stmt(Codegen *cg, ASTNode *node);
static void codegen_block(Codegen *cg, ASTNode *node);
static LLVMTypeRef type_to_llvm(Codegen *cg, Type *type);

// Scope management
static CGScope *cgscope_new(CGScope *parent) {
    CGScope *s = calloc(1, sizeof(CGScope));
    s->parent = parent;
    s->symbols = NULL;
    return s;
}

static void cgscope_free(CGScope *s) {
    if (!s) return;
    CGSymbol *sym = s->symbols;
    while (sym) {
        CGSymbol *next = sym->next;
        free(sym->name);
        free(sym);
        sym = next;
    }
    free(s);
}

static CGSymbol *cgscope_lookup(CGScope *s, const char *name) {
    while (s) {
        for (CGSymbol *sym = s->symbols; sym; sym = sym->next) {
            if (strcmp(sym->name, name) == 0) return sym;
        }
        s = s->parent;
    }
    return NULL;
}

static void cgscope_define(CGScope *s, const char *name, LLVMValueRef value, LLVMTypeRef llvm_type, Type *type, bool is_ptr) {
    CGSymbol *sym = calloc(1, sizeof(CGSymbol));
    sym->name = strdup(name);
    sym->value = value;
    sym->llvm_type = llvm_type;
    sym->type = type;
    sym->is_ptr = is_ptr;
    sym->next = s->symbols;
    s->symbols = sym;
}

static void error(Codegen *cg, const char *msg) {
    if (cg->had_error) return;
    cg->had_error = true;
    fprintf(stderr, "Codegen error: %s\n", msg);
}

void codegen_init(Codegen *cg, const char *module_name) {
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();

    cg->context = LLVMContextCreate();
    cg->module = LLVMModuleCreateWithNameInContext(module_name, cg->context);
    cg->builder = LLVMCreateBuilderInContext(cg->context);

    // Set up target
    char *triple = LLVMGetDefaultTargetTriple();
    LLVMSetTarget(cg->module, triple);

    LLVMTargetRef target;
    char *error_msg = NULL;
    if (LLVMGetTargetFromTriple(triple, &target, &error_msg)) {
        fprintf(stderr, "Error getting target: %s\n", error_msg);
        LLVMDisposeMessage(error_msg);
    }

    cg->target_machine = LLVMCreateTargetMachine(
        target, triple, "generic", "",
        LLVMCodeGenLevelDefault,
        LLVMRelocPIC,
        LLVMCodeModelDefault
    );

    LLVMDisposeMessage(triple);

    cg->global_scope = cgscope_new(NULL);
    cg->current_scope = cg->global_scope;
    cg->loop_exit = NULL;
    cg->loop_continue = NULL;
    cg->current_fn_ret_type = NULL;
    cg->string_count = 0;
    cg->had_error = false;
    cg->error_msg = NULL;
}

void codegen_free(Codegen *cg) {
    if (cg->target_machine) LLVMDisposeTargetMachine(cg->target_machine);
    if (cg->builder) LLVMDisposeBuilder(cg->builder);
    if (cg->module) LLVMDisposeModule(cg->module);
    if (cg->context) LLVMContextDispose(cg->context);

    CGScope *s = cg->global_scope;
    while (s) {
        CGScope *next = s->parent;
        cgscope_free(s);
        s = next;
    }
}

static LLVMTypeRef type_to_llvm(Codegen *cg, Type *type) {
    if (!type) return LLVMInt64TypeInContext(cg->context);

    switch (type->kind) {
        case TYPE_VOID:
            return LLVMVoidTypeInContext(cg->context);
        case TYPE_BOOL:
            return LLVMInt1TypeInContext(cg->context);
        case TYPE_I8:
        case TYPE_U8:
            return LLVMInt8TypeInContext(cg->context);
        case TYPE_I16:
        case TYPE_U16:
            return LLVMInt16TypeInContext(cg->context);
        case TYPE_I32:
        case TYPE_U32:
            return LLVMInt32TypeInContext(cg->context);
        case TYPE_I64:
        case TYPE_U64:
            return LLVMInt64TypeInContext(cg->context);
        case TYPE_F32:
            return LLVMFloatTypeInContext(cg->context);
        case TYPE_F64:
            return LLVMDoubleTypeInContext(cg->context);
        case TYPE_PTR:
            return LLVMPointerTypeInContext(cg->context, 0);
        case TYPE_ARRAY: {
            LLVMTypeRef elem = type_to_llvm(cg, type->array.elem_type);
            return LLVMArrayType2(elem, type->array.size);
        }
        case TYPE_SLICE:
            // Slice is pointer to element type
            return LLVMPointerTypeInContext(cg->context, 0);
        case TYPE_STRUCT: {
            // Look up or create struct type
            LLVMTypeRef struct_type = LLVMGetTypeByName2(cg->context, type->struct_t.name);
            if (!struct_type) {
                // Create opaque struct
                struct_type = LLVMStructCreateNamed(cg->context, type->struct_t.name);
                if (type->struct_t.field_count > 0) {
                    LLVMTypeRef *field_types = malloc(sizeof(LLVMTypeRef) * type->struct_t.field_count);
                    for (int i = 0; i < type->struct_t.field_count; i++) {
                        field_types[i] = type_to_llvm(cg, type->struct_t.field_types[i]);
                    }
                    LLVMStructSetBody(struct_type, field_types, type->struct_t.field_count, 0);
                    free(field_types);
                }
            }
            return struct_type;
        }
        case TYPE_FN: {
            LLVMTypeRef ret = type_to_llvm(cg, type->fn.ret_type);
            LLVMTypeRef *params = malloc(sizeof(LLVMTypeRef) * type->fn.param_count);
            for (int i = 0; i < type->fn.param_count; i++) {
                params[i] = type_to_llvm(cg, type->fn.param_types[i]);
            }
            LLVMTypeRef fn_type = LLVMFunctionType(ret, params, type->fn.param_count, 0);
            free(params);
            return fn_type;
        }
        default:
            return LLVMInt64TypeInContext(cg->context);
    }
}

static void codegen_fn_decl(Codegen *cg, ASTNode *node) {
    // Build function type
    LLVMTypeRef ret_type = type_to_llvm(cg, node->fn_decl.ret_type);
    LLVMTypeRef *param_types = malloc(sizeof(LLVMTypeRef) * node->fn_decl.param_count);
    for (int i = 0; i < node->fn_decl.param_count; i++) {
        param_types[i] = type_to_llvm(cg, node->fn_decl.params[i]->param.param_type);
    }
    LLVMTypeRef fn_type = LLVMFunctionType(ret_type, param_types, node->fn_decl.param_count, 0);

    // Create function
    LLVMValueRef fn = LLVMGetNamedFunction(cg->module, node->fn_decl.name);
    if (!fn) {
        fn = LLVMAddFunction(cg->module, node->fn_decl.name, fn_type);
    }

    // Register in scope
    cgscope_define(cg->global_scope, node->fn_decl.name, fn, fn_type, NULL, false);

    if (node->fn_decl.is_extern || !node->fn_decl.body) {
        free(param_types);
        return;
    }

    // Create entry block
    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(cg->context, fn, "entry");
    LLVMPositionBuilderAtEnd(cg->builder, entry);

    // Create function scope
    CGScope *fn_scope = cgscope_new(cg->current_scope);
    cg->current_scope = fn_scope;
    cg->current_fn_ret_type = ret_type;

    // Add parameters to scope
    for (int i = 0; i < node->fn_decl.param_count; i++) {
        LLVMValueRef param_val = LLVMGetParam(fn, i);
        ASTNode *param = node->fn_decl.params[i];
        LLVMSetValueName2(param_val, param->param.name, strlen(param->param.name));

        // Allocate space for parameter
        LLVMValueRef alloca = LLVMBuildAlloca(cg->builder, param_types[i], param->param.name);
        LLVMBuildStore(cg->builder, param_val, alloca);
        cgscope_define(fn_scope, param->param.name, alloca, param_types[i], param->param.param_type, true);
    }

    // Generate body
    codegen_block(cg, node->fn_decl.body);

    // Add implicit return if needed
    LLVMBasicBlockRef current_bb = LLVMGetInsertBlock(cg->builder);
    if (!LLVMGetBasicBlockTerminator(current_bb)) {
        if (node->fn_decl.ret_type->kind == TYPE_VOID) {
            LLVMBuildRetVoid(cg->builder);
        } else {
            LLVMBuildRet(cg->builder, LLVMConstInt(type_to_llvm(cg, node->fn_decl.ret_type), 0, 0));
        }
    }

    cg->current_scope = fn_scope->parent;
    cg->current_fn_ret_type = NULL;
    cgscope_free(fn_scope);
    free(param_types);

    // Verify function
    char *err = NULL;
    if (LLVMVerifyFunction(fn, LLVMPrintMessageAction)) {
        fprintf(stderr, "Function verification failed for %s\n", node->fn_decl.name);
    }
}

static void codegen_struct_decl(Codegen *cg, ASTNode *node) {
    // Create struct type
    LLVMTypeRef struct_type = LLVMStructCreateNamed(cg->context, node->struct_decl.name);

    LLVMTypeRef *field_types = malloc(sizeof(LLVMTypeRef) * node->struct_decl.field_count);
    for (int i = 0; i < node->struct_decl.field_count; i++) {
        field_types[i] = type_to_llvm(cg, node->struct_decl.field_types[i]);
    }

    LLVMStructSetBody(struct_type, field_types, node->struct_decl.field_count, 0);
    free(field_types);
}

static void codegen_var_decl(Codegen *cg, ASTNode *node) {
    LLVMTypeRef var_type = type_to_llvm(cg, node->var_decl.var_type);

    // Allocate space
    LLVMValueRef alloca = LLVMBuildAlloca(cg->builder, var_type, node->var_decl.name);

    // Initialize
    if (node->var_decl.init) {
        LLVMValueRef init_val = codegen_expr(cg, node->var_decl.init);
        if (init_val) {
            LLVMBuildStore(cg->builder, init_val, alloca);
        }
    }

    cgscope_define(cg->current_scope, node->var_decl.name, alloca, var_type, node->var_decl.var_type, true);
}

static void codegen_block(Codegen *cg, ASTNode *node) {
    CGScope *block_scope = cgscope_new(cg->current_scope);
    cg->current_scope = block_scope;

    for (int i = 0; i < node->block.stmt_count; i++) {
        codegen_stmt(cg, node->block.stmts[i]);

        // Stop if we hit a terminator
        LLVMBasicBlockRef bb = LLVMGetInsertBlock(cg->builder);
        if (bb && LLVMGetBasicBlockTerminator(bb)) {
            break;
        }
    }

    cg->current_scope = block_scope->parent;
    cgscope_free(block_scope);
}

static void codegen_stmt(Codegen *cg, ASTNode *node) {
    if (!node) return;

    switch (node->kind) {
        case NODE_VAR_DECL:
            codegen_var_decl(cg, node);
            break;

        case NODE_RETURN: {
            if (node->ret.value) {
                LLVMValueRef val = codegen_expr(cg, node->ret.value);
                // Cast to expected return type if needed
                if (cg->current_fn_ret_type) {
                    LLVMTypeRef val_type = LLVMTypeOf(val);
                    if (val_type != cg->current_fn_ret_type) {
                        LLVMTypeKind ret_kind = LLVMGetTypeKind(cg->current_fn_ret_type);
                        LLVMTypeKind val_kind = LLVMGetTypeKind(val_type);
                        if (ret_kind == LLVMIntegerTypeKind && val_kind == LLVMIntegerTypeKind) {
                            val = LLVMBuildIntCast2(cg->builder, val, cg->current_fn_ret_type, 1, "ret_cast");
                        } else if (ret_kind == LLVMFloatTypeKind || ret_kind == LLVMDoubleTypeKind) {
                            if (val_kind == LLVMIntegerTypeKind) {
                                val = LLVMBuildSIToFP(cg->builder, val, cg->current_fn_ret_type, "itof");
                            } else {
                                val = LLVMBuildFPCast(cg->builder, val, cg->current_fn_ret_type, "fcast");
                            }
                        }
                    }
                }
                LLVMBuildRet(cg->builder, val);
            } else {
                LLVMBuildRetVoid(cg->builder);
            }
            break;
        }

        case NODE_IF: {
            LLVMValueRef fn = LLVMGetBasicBlockParent(LLVMGetInsertBlock(cg->builder));

            LLVMBasicBlockRef then_bb = LLVMAppendBasicBlockInContext(cg->context, fn, "then");
            LLVMBasicBlockRef else_bb = LLVMAppendBasicBlockInContext(cg->context, fn, "else");
            LLVMBasicBlockRef merge_bb = LLVMAppendBasicBlockInContext(cg->context, fn, "merge");

            LLVMValueRef cond = codegen_expr(cg, node->if_stmt.cond);
            LLVMBuildCondBr(cg->builder, cond, then_bb, else_bb);

            // Then block
            LLVMPositionBuilderAtEnd(cg->builder, then_bb);
            codegen_block(cg, node->if_stmt.then_block);
            if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(cg->builder))) {
                LLVMBuildBr(cg->builder, merge_bb);
            }

            // Elif/else handling
            LLVMPositionBuilderAtEnd(cg->builder, else_bb);

            for (int i = 0; i < node->if_stmt.elif_count; i++) {
                LLVMBasicBlockRef elif_then = LLVMAppendBasicBlockInContext(cg->context, fn, "elif_then");
                LLVMBasicBlockRef elif_else = LLVMAppendBasicBlockInContext(cg->context, fn, "elif_else");

                LLVMValueRef elif_cond = codegen_expr(cg, node->if_stmt.elif_conds[i]);
                LLVMBuildCondBr(cg->builder, elif_cond, elif_then, elif_else);

                LLVMPositionBuilderAtEnd(cg->builder, elif_then);
                codegen_block(cg, node->if_stmt.elif_blocks[i]);
                if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(cg->builder))) {
                    LLVMBuildBr(cg->builder, merge_bb);
                }

                LLVMPositionBuilderAtEnd(cg->builder, elif_else);
            }

            if (node->if_stmt.else_block) {
                codegen_block(cg, node->if_stmt.else_block);
            }
            if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(cg->builder))) {
                LLVMBuildBr(cg->builder, merge_bb);
            }

            LLVMPositionBuilderAtEnd(cg->builder, merge_bb);
            break;
        }

        case NODE_WHILE: {
            LLVMValueRef fn = LLVMGetBasicBlockParent(LLVMGetInsertBlock(cg->builder));

            LLVMBasicBlockRef cond_bb = LLVMAppendBasicBlockInContext(cg->context, fn, "while_cond");
            LLVMBasicBlockRef body_bb = LLVMAppendBasicBlockInContext(cg->context, fn, "while_body");
            LLVMBasicBlockRef end_bb = LLVMAppendBasicBlockInContext(cg->context, fn, "while_end");

            LLVMBasicBlockRef old_exit = cg->loop_exit;
            LLVMBasicBlockRef old_continue = cg->loop_continue;
            cg->loop_exit = end_bb;
            cg->loop_continue = cond_bb;

            LLVMBuildBr(cg->builder, cond_bb);

            LLVMPositionBuilderAtEnd(cg->builder, cond_bb);
            LLVMValueRef cond = codegen_expr(cg, node->while_stmt.cond);
            LLVMBuildCondBr(cg->builder, cond, body_bb, end_bb);

            LLVMPositionBuilderAtEnd(cg->builder, body_bb);
            codegen_block(cg, node->while_stmt.body);
            if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(cg->builder))) {
                LLVMBuildBr(cg->builder, cond_bb);
            }

            LLVMPositionBuilderAtEnd(cg->builder, end_bb);

            cg->loop_exit = old_exit;
            cg->loop_continue = old_continue;
            break;
        }

        case NODE_FOR: {
            LLVMValueRef fn = LLVMGetBasicBlockParent(LLVMGetInsertBlock(cg->builder));

            // Create loop variable
            CGScope *loop_scope = cgscope_new(cg->current_scope);
            cg->current_scope = loop_scope;

            LLVMTypeRef i64_type = LLVMInt64TypeInContext(cg->context);
            LLVMValueRef iter_alloca = LLVMBuildAlloca(cg->builder, i64_type, node->for_stmt.var_name);

            LLVMValueRef start_val = codegen_expr(cg, node->for_stmt.start);
            LLVMBuildStore(cg->builder, start_val, iter_alloca);

            cgscope_define(loop_scope, node->for_stmt.var_name, iter_alloca, i64_type, NULL, true);

            LLVMBasicBlockRef cond_bb = LLVMAppendBasicBlockInContext(cg->context, fn, "for_cond");
            LLVMBasicBlockRef body_bb = LLVMAppendBasicBlockInContext(cg->context, fn, "for_body");
            LLVMBasicBlockRef inc_bb = LLVMAppendBasicBlockInContext(cg->context, fn, "for_inc");
            LLVMBasicBlockRef end_bb = LLVMAppendBasicBlockInContext(cg->context, fn, "for_end");

            LLVMBasicBlockRef old_exit = cg->loop_exit;
            LLVMBasicBlockRef old_continue = cg->loop_continue;
            cg->loop_exit = end_bb;
            cg->loop_continue = inc_bb;

            LLVMBuildBr(cg->builder, cond_bb);

            // Condition
            LLVMPositionBuilderAtEnd(cg->builder, cond_bb);
            LLVMValueRef iter_val = LLVMBuildLoad2(cg->builder, i64_type, iter_alloca, "iter");
            LLVMValueRef end_val = codegen_expr(cg, node->for_stmt.end);
            LLVMValueRef cond = LLVMBuildICmp(cg->builder, LLVMIntSLT, iter_val, end_val, "cond");
            LLVMBuildCondBr(cg->builder, cond, body_bb, end_bb);

            // Body
            LLVMPositionBuilderAtEnd(cg->builder, body_bb);
            codegen_block(cg, node->for_stmt.body);
            if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(cg->builder))) {
                LLVMBuildBr(cg->builder, inc_bb);
            }

            // Increment
            LLVMPositionBuilderAtEnd(cg->builder, inc_bb);
            LLVMValueRef cur_val = LLVMBuildLoad2(cg->builder, i64_type, iter_alloca, "cur");
            LLVMValueRef next_val = LLVMBuildAdd(cg->builder, cur_val, LLVMConstInt(i64_type, 1, 0), "next");
            LLVMBuildStore(cg->builder, next_val, iter_alloca);
            LLVMBuildBr(cg->builder, cond_bb);

            LLVMPositionBuilderAtEnd(cg->builder, end_bb);

            cg->loop_exit = old_exit;
            cg->loop_continue = old_continue;
            cg->current_scope = loop_scope->parent;
            cgscope_free(loop_scope);
            break;
        }

        case NODE_ASSIGN: {
            LLVMValueRef val = codegen_expr(cg, node->assign.value);
            if (node->assign.target->kind == NODE_IDENT) {
                CGSymbol *sym = cgscope_lookup(cg->current_scope, node->assign.target->ident);
                if (sym && sym->is_ptr) {
                    LLVMBuildStore(cg->builder, val, sym->value);
                }
            } else if (node->assign.target->kind == NODE_MEMBER) {
                // Struct member assignment
                // TODO: implement
            } else if (node->assign.target->kind == NODE_INDEX) {
                // Array index assignment
                // TODO: implement
            }
            break;
        }

        case NODE_EXPR_STMT:
            codegen_expr(cg, node->expr_stmt.expr);
            break;

        default:
            break;
    }
}

static LLVMValueRef codegen_expr(Codegen *cg, ASTNode *node) {
    if (!node) return NULL;

    switch (node->kind) {
        case NODE_LITERAL_INT:
            return LLVMConstInt(LLVMInt64TypeInContext(cg->context), node->int_val, 1);

        case NODE_LITERAL_FLOAT:
            return LLVMConstReal(LLVMDoubleTypeInContext(cg->context), node->float_val);

        case NODE_LITERAL_STRING: {
            // Create global string constant
            size_t len = strlen(node->string_val);
            LLVMValueRef str = LLVMBuildGlobalStringPtr(cg->builder, node->string_val, "str");
            return str;
        }

        case NODE_LITERAL_BOOL:
            return LLVMConstInt(LLVMInt1TypeInContext(cg->context), node->bool_val ? 1 : 0, 0);

        case NODE_IDENT: {
            CGSymbol *sym = cgscope_lookup(cg->current_scope, node->ident);
            if (!sym) {
                char msg[256];
                snprintf(msg, sizeof(msg), "Unknown identifier: %s", node->ident);
                error(cg, msg);
                return LLVMConstInt(LLVMInt64TypeInContext(cg->context), 0, 0);
            }
            if (sym->is_ptr) {
                return LLVMBuildLoad2(cg->builder, sym->llvm_type, sym->value, node->ident);
            }
            return sym->value;
        }

        case NODE_BINARY: {
            LLVMValueRef left = codegen_expr(cg, node->binary.left);
            LLVMValueRef right = codegen_expr(cg, node->binary.right);

            LLVMTypeRef left_type = LLVMTypeOf(left);
            bool is_float = LLVMGetTypeKind(left_type) == LLVMFloatTypeKind ||
                           LLVMGetTypeKind(left_type) == LLVMDoubleTypeKind;

            switch (node->binary.op) {
                case BIN_ADD:
                    return is_float ? LLVMBuildFAdd(cg->builder, left, right, "fadd")
                                   : LLVMBuildAdd(cg->builder, left, right, "add");
                case BIN_SUB:
                    return is_float ? LLVMBuildFSub(cg->builder, left, right, "fsub")
                                   : LLVMBuildSub(cg->builder, left, right, "sub");
                case BIN_MUL:
                    return is_float ? LLVMBuildFMul(cg->builder, left, right, "fmul")
                                   : LLVMBuildMul(cg->builder, left, right, "mul");
                case BIN_DIV:
                    return is_float ? LLVMBuildFDiv(cg->builder, left, right, "fdiv")
                                   : LLVMBuildSDiv(cg->builder, left, right, "sdiv");
                case BIN_MOD:
                    return LLVMBuildSRem(cg->builder, left, right, "mod");
                case BIN_EQ:
                    return is_float ? LLVMBuildFCmp(cg->builder, LLVMRealOEQ, left, right, "feq")
                                   : LLVMBuildICmp(cg->builder, LLVMIntEQ, left, right, "eq");
                case BIN_NE:
                    return is_float ? LLVMBuildFCmp(cg->builder, LLVMRealONE, left, right, "fne")
                                   : LLVMBuildICmp(cg->builder, LLVMIntNE, left, right, "ne");
                case BIN_LT:
                    return is_float ? LLVMBuildFCmp(cg->builder, LLVMRealOLT, left, right, "flt")
                                   : LLVMBuildICmp(cg->builder, LLVMIntSLT, left, right, "lt");
                case BIN_LE:
                    return is_float ? LLVMBuildFCmp(cg->builder, LLVMRealOLE, left, right, "fle")
                                   : LLVMBuildICmp(cg->builder, LLVMIntSLE, left, right, "le");
                case BIN_GT:
                    return is_float ? LLVMBuildFCmp(cg->builder, LLVMRealOGT, left, right, "fgt")
                                   : LLVMBuildICmp(cg->builder, LLVMIntSGT, left, right, "gt");
                case BIN_GE:
                    return is_float ? LLVMBuildFCmp(cg->builder, LLVMRealOGE, left, right, "fge")
                                   : LLVMBuildICmp(cg->builder, LLVMIntSGE, left, right, "ge");
                case BIN_AND:
                    return LLVMBuildAnd(cg->builder, left, right, "and");
                case BIN_OR:
                    return LLVMBuildOr(cg->builder, left, right, "or");
                case BIN_BAND:
                    return LLVMBuildAnd(cg->builder, left, right, "band");
                case BIN_BOR:
                    return LLVMBuildOr(cg->builder, left, right, "bor");
                case BIN_BXOR:
                    return LLVMBuildXor(cg->builder, left, right, "bxor");
                case BIN_LSHIFT:
                    return LLVMBuildShl(cg->builder, left, right, "shl");
                case BIN_RSHIFT:
                    return LLVMBuildAShr(cg->builder, left, right, "shr");
                default:
                    return left;
            }
        }

        case NODE_UNARY: {
            LLVMValueRef operand = codegen_expr(cg, node->unary.operand);
            switch (node->unary.op) {
                case UN_NEG:
                    return LLVMBuildNeg(cg->builder, operand, "neg");
                case UN_NOT:
                    return LLVMBuildNot(cg->builder, operand, "not");
                case UN_BNOT:
                    return LLVMBuildNot(cg->builder, operand, "bnot");
                default:
                    return operand;
            }
        }

        case NODE_CALL: {
            // Get function
            LLVMValueRef fn = NULL;
            LLVMTypeRef fn_type = NULL;
            const char *fn_name = NULL;

            if (node->call.callee->kind == NODE_IDENT) {
                fn_name = node->call.callee->ident;
                CGSymbol *sym = cgscope_lookup(cg->current_scope, fn_name);
                if (sym) {
                    fn = sym->value;
                    fn_type = sym->llvm_type;
                } else {
                    fn = LLVMGetNamedFunction(cg->module, fn_name);
                    if (fn) {
                        fn_type = LLVMGlobalGetValueType(fn);
                    }
                }
            } else if (node->call.callee->kind == NODE_MEMBER) {
                // Module.function call
                ASTNode *member = node->call.callee;
                if (member->member.object->kind == NODE_IDENT) {
                    // Try to find fully qualified name
                    char full_name[256];
                    snprintf(full_name, sizeof(full_name), "%s_%s",
                             member->member.object->ident, member->member.member);
                    fn = LLVMGetNamedFunction(cg->module, full_name);
                    if (fn) {
                        fn_type = LLVMGlobalGetValueType(fn);
                        fn_name = full_name;
                    }
                }
            }

            if (!fn) {
                char msg[256];
                snprintf(msg, sizeof(msg), "Unknown function in call");
                error(cg, msg);
                return LLVMConstInt(LLVMInt64TypeInContext(cg->context), 0, 0);
            }

            // Build args
            LLVMValueRef *args = malloc(sizeof(LLVMValueRef) * node->call.arg_count);
            for (int i = 0; i < node->call.arg_count; i++) {
                args[i] = codegen_expr(cg, node->call.args[i]);
            }

            LLVMValueRef result = LLVMBuildCall2(cg->builder, fn_type, fn, args, node->call.arg_count, "");
            free(args);
            return result;
        }

        case NODE_MEMBER: {
            // Struct field access
            LLVMValueRef obj = codegen_expr(cg, node->member.object);
            // TODO: proper struct field GEP
            return obj;
        }

        case NODE_INDEX: {
            LLVMValueRef arr = codegen_expr(cg, node->index.object);
            LLVMValueRef idx = codegen_expr(cg, node->index.index);
            // TODO: proper array indexing with GEP
            (void)idx;
            return arr;
        }

        case NODE_ASSIGN: {
            // Handle assignment as expression
            LLVMValueRef val = codegen_expr(cg, node->assign.value);
            if (node->assign.target->kind == NODE_IDENT) {
                CGSymbol *sym = cgscope_lookup(cg->current_scope, node->assign.target->ident);
                if (sym && sym->is_ptr) {
                    LLVMBuildStore(cg->builder, val, sym->value);
                }
            }
            return val;
        }

        case NODE_STRUCT_INIT: {
            // Create struct value
            LLVMTypeRef struct_type = LLVMGetTypeByName2(cg->context, node->struct_init.struct_name);
            if (!struct_type) {
                error(cg, "Unknown struct type");
                return LLVMConstInt(LLVMInt64TypeInContext(cg->context), 0, 0);
            }

            // Allocate struct
            LLVMValueRef alloca = LLVMBuildAlloca(cg->builder, struct_type, "struct_tmp");

            // Initialize fields
            for (int i = 0; i < node->struct_init.field_count; i++) {
                LLVMValueRef field_ptr = LLVMBuildStructGEP2(cg->builder, struct_type, alloca, i, "field_ptr");
                LLVMValueRef field_val = codegen_expr(cg, node->struct_init.field_values[i]);
                LLVMBuildStore(cg->builder, field_val, field_ptr);
            }

            return LLVMBuildLoad2(cg->builder, struct_type, alloca, "struct_val");
        }

        case NODE_ARRAY_INIT: {
            // Create array value
            if (node->array_init.elem_count == 0) {
                return LLVMConstNull(LLVMPointerTypeInContext(cg->context, 0));
            }

            LLVMValueRef first = codegen_expr(cg, node->array_init.elements[0]);
            LLVMTypeRef elem_type = LLVMTypeOf(first);
            LLVMTypeRef arr_type = LLVMArrayType2(elem_type, node->array_init.elem_count);

            LLVMValueRef alloca = LLVMBuildAlloca(cg->builder, arr_type, "arr_tmp");

            for (int i = 0; i < node->array_init.elem_count; i++) {
                LLVMValueRef indices[2] = {
                    LLVMConstInt(LLVMInt64TypeInContext(cg->context), 0, 0),
                    LLVMConstInt(LLVMInt64TypeInContext(cg->context), i, 0)
                };
                LLVMValueRef elem_ptr = LLVMBuildGEP2(cg->builder, arr_type, alloca, indices, 2, "elem_ptr");
                LLVMValueRef elem_val = (i == 0) ? first : codegen_expr(cg, node->array_init.elements[i]);
                LLVMBuildStore(cg->builder, elem_val, elem_ptr);
            }

            return alloca;
        }

        default:
            return LLVMConstInt(LLVMInt64TypeInContext(cg->context), 0, 0);
    }
}

bool codegen_generate(Codegen *cg, ASTNode *ast) {
    if (!ast || ast->kind != NODE_PROGRAM) {
        return false;
    }

    // First pass: declare all structs
    for (int i = 0; i < ast->program.decl_count; i++) {
        ASTNode *decl = ast->program.decls[i];
        if (decl->kind == NODE_STRUCT_DECL) {
            codegen_struct_decl(cg, decl);
        }
    }

    // Second pass: declare all functions
    for (int i = 0; i < ast->program.decl_count; i++) {
        ASTNode *decl = ast->program.decls[i];
        if (decl->kind == NODE_FN_DECL) {
            // Just declare, don't generate body yet
            LLVMTypeRef ret_type = type_to_llvm(cg, decl->fn_decl.ret_type);
            LLVMTypeRef *param_types = malloc(sizeof(LLVMTypeRef) * decl->fn_decl.param_count);
            for (int j = 0; j < decl->fn_decl.param_count; j++) {
                param_types[j] = type_to_llvm(cg, decl->fn_decl.params[j]->param.param_type);
            }
            LLVMTypeRef fn_type = LLVMFunctionType(ret_type, param_types, decl->fn_decl.param_count, 0);
            LLVMAddFunction(cg->module, decl->fn_decl.name, fn_type);
            free(param_types);
        } else if (decl->kind == NODE_EXTERN) {
            for (int j = 0; j < decl->extern_decl.fn_count; j++) {
                ASTNode *fn = decl->extern_decl.fn_decls[j];
                LLVMTypeRef ret_type = type_to_llvm(cg, fn->fn_decl.ret_type);
                LLVMTypeRef *param_types = malloc(sizeof(LLVMTypeRef) * fn->fn_decl.param_count);
                for (int k = 0; k < fn->fn_decl.param_count; k++) {
                    param_types[k] = type_to_llvm(cg, fn->fn_decl.params[k]->param.param_type);
                }
                LLVMTypeRef fn_type = LLVMFunctionType(ret_type, param_types, fn->fn_decl.param_count, 0);
                LLVMAddFunction(cg->module, fn->fn_decl.name, fn_type);
                free(param_types);
            }
        }
    }

    // Third pass: generate function bodies
    for (int i = 0; i < ast->program.decl_count; i++) {
        ASTNode *decl = ast->program.decls[i];
        if (decl->kind == NODE_FN_DECL) {
            codegen_fn_decl(cg, decl);
        }
    }

    // Verify module
    char *err = NULL;
    if (LLVMVerifyModule(cg->module, LLVMPrintMessageAction, &err)) {
        fprintf(stderr, "Module verification failed: %s\n", err);
        LLVMDisposeMessage(err);
        return false;
    }

    return !cg->had_error;
}

bool codegen_emit_object(Codegen *cg, const char *filename) {
    char *err = NULL;
    if (LLVMTargetMachineEmitToFile(cg->target_machine, cg->module, (char*)filename, LLVMObjectFile, &err)) {
        fprintf(stderr, "Error emitting object file: %s\n", err);
        LLVMDisposeMessage(err);
        return false;
    }
    return true;
}

void codegen_emit_ir(Codegen *cg, const char *filename) {
    LLVMPrintModuleToFile(cg->module, filename, NULL);
}

int codegen_jit_run(Codegen *cg) {
    LLVMExecutionEngineRef engine;
    char *err = NULL;

    LLVMLinkInMCJIT();

    if (LLVMCreateExecutionEngineForModule(&engine, cg->module, &err)) {
        fprintf(stderr, "Failed to create JIT: %s\n", err);
        LLVMDisposeMessage(err);
        return 1;
    }

    LLVMValueRef main_fn = LLVMGetNamedFunction(cg->module, "main");
    if (!main_fn) {
        fprintf(stderr, "No main function found\n");
        LLVMDisposeExecutionEngine(engine);
        return 1;
    }

    int result = (int)LLVMRunFunctionAsMain(engine, main_fn, 0, NULL, NULL);

    // Don't dispose module - engine owns it now
    cg->module = NULL;
    LLVMDisposeExecutionEngine(engine);

    return result;
}
