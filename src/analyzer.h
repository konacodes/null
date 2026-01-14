#ifndef NULL_ANALYZER_H
#define NULL_ANALYZER_H

#include "parser.h"

// Symbol kinds
typedef enum {
    SYM_VAR,
    SYM_FN,
    SYM_STRUCT,
    SYM_ENUM,
    SYM_PARAM,
} SymbolKind;

// Symbol entry
typedef struct Symbol {
    char *name;
    SymbolKind kind;
    Type *type;
    bool is_mut;
    bool is_extern;
    ASTNode *decl;          // pointer to declaration node
    struct Symbol *next;    // for chaining in scope
} Symbol;

// Scope for symbol table
typedef struct Scope {
    Symbol *symbols;
    struct Scope *parent;
} Scope;

// Analyzer state
typedef struct {
    Scope *global_scope;
    Scope *current_scope;
    Type *current_fn_ret_type;
    bool had_error;
    char *error_msg;
} Analyzer;

// Initialize analyzer
void analyzer_init(Analyzer *a);

// Analyze AST
bool analyzer_analyze(Analyzer *a, ASTNode *ast);

// Free analyzer
void analyzer_free(Analyzer *a);

// Scope management
Scope *scope_new(Scope *parent);
void scope_free(Scope *s);
Symbol *scope_lookup(Scope *s, const char *name);
Symbol *scope_lookup_local(Scope *s, const char *name);
void scope_define(Scope *s, Symbol *sym);

// Symbol creation
Symbol *symbol_new(const char *name, SymbolKind kind, Type *type);
void symbol_free(Symbol *sym);

#endif // NULL_ANALYZER_H
