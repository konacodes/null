#ifndef NULL_CODEGEN_H
#define NULL_CODEGEN_H

#include "parser.h"
#include "analyzer.h"
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Transforms/PassBuilder.h>

// Symbol value for codegen
typedef struct CGSymbol {
    char *name;
    LLVMValueRef value;
    LLVMTypeRef llvm_type;
    Type *type;
    bool is_ptr;    // is this an alloca (needs load)?
    struct CGSymbol *next;
} CGSymbol;

// Scope for codegen
typedef struct CGScope {
    CGSymbol *symbols;
    struct CGScope *parent;
} CGScope;

// Struct definition registry
typedef struct {
    char *name;
    int field_count;
    char **field_names;
    Type **field_types;
} StructDef;

// Enum definition registry
typedef struct {
    char *name;
    int variant_count;
    char **variant_names;
    int64_t *variant_values;
} EnumDef;

// Codegen context
typedef struct {
    LLVMContextRef context;
    LLVMModuleRef module;
    LLVMBuilderRef builder;
    LLVMTargetMachineRef target_machine;

    CGScope *global_scope;
    CGScope *current_scope;

    // For break/continue support
    LLVMBasicBlockRef loop_exit;
    LLVMBasicBlockRef loop_continue;

    // Current function context
    LLVMTypeRef current_fn_ret_type;
    LLVMValueRef current_fn_sret;     // sret pointer for struct returns (NULL if not struct return)
    Type *current_fn_ret_null_type;   // null language type for return

    // Struct registry
    StructDef *structs;
    int struct_count;

    // Enum registry
    EnumDef *enums;
    int enum_count;

    // String constants cache
    int string_count;

    bool had_error;
    char *error_msg;
} Codegen;

// Initialize codegen
void codegen_init(Codegen *cg, const char *module_name);

// Generate code from AST
bool codegen_generate(Codegen *cg, ASTNode *ast);

// Emit object file
bool codegen_emit_object(Codegen *cg, const char *filename);

// Emit LLVM IR (for debugging)
void codegen_emit_ir(Codegen *cg, const char *filename);

// JIT execute
int codegen_jit_run(Codegen *cg);

// Free codegen
void codegen_free(Codegen *cg);

#endif // NULL_CODEGEN_H
