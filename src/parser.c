#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>

// Memory helpers
static char *str_dup(const char *s, size_t len) {
    char *d = malloc(len + 1);
    if (!d) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    memcpy(d, s, len);
    d[len] = '\0';
    return d;
}

// Duplicate string with escape sequence interpretation
static char *str_dup_unescape(const char *s, size_t len) {
    char *d = malloc(len + 1);
    if (!d) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (s[i] == '\\' && i + 1 < len) {
            i++;
            switch (s[i]) {
                case 'n':  d[j++] = '\n'; break;
                case 't':  d[j++] = '\t'; break;
                case 'r':  d[j++] = '\r'; break;
                case '\\': d[j++] = '\\'; break;
                case '"':  d[j++] = '"';  break;
                case '0':  d[j++] = '\0'; break;
                default:   d[j++] = s[i]; break;  // Unknown escape, keep as-is
            }
        } else {
            d[j++] = s[i];
        }
    }
    d[j] = '\0';
    return d;
}

// Safe realloc helper - exits on failure
static void *safe_realloc(void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr && size > 0) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return new_ptr;
}

// Ensure array has capacity for one more element (geometric growth)
#define ENSURE_CAPACITY(arr, count, capacity, type) do { \
    if ((count) >= (capacity)) { \
        (capacity) = (capacity) ? (capacity) * 2 : 8; \
        (arr) = safe_realloc((arr), sizeof(type) * (capacity)); \
    } \
} while(0)

// Type utilities
Type *type_new(TypeKind kind) {
    Type *t = calloc(1, sizeof(Type));
    t->kind = kind;
    t->line = 0;
    t->column = 0;
    return t;
}

Type *type_new_at(TypeKind kind, int line, int column) {
    Type *t = calloc(1, sizeof(Type));
    t->kind = kind;
    t->line = line;
    t->column = column;
    return t;
}

Type *type_clone(Type *t) {
    if (!t) return NULL;
    Type *c = type_new(t->kind);
    c->line = t->line;
    c->column = t->column;
    switch (t->kind) {
        case TYPE_PTR:
            c->ptr_to = type_clone(t->ptr_to);
            break;
        case TYPE_ARRAY:
        case TYPE_SLICE:
            c->array.elem_type = type_clone(t->array.elem_type);
            c->array.size = t->array.size;
            break;
        case TYPE_STRUCT:
            c->struct_t.name = t->struct_t.name ? strdup(t->struct_t.name) : NULL;
            c->struct_t.field_count = t->struct_t.field_count;
            if (t->struct_t.field_count > 0) {
                c->struct_t.field_names = malloc(sizeof(char*) * t->struct_t.field_count);
                c->struct_t.field_types = malloc(sizeof(Type*) * t->struct_t.field_count);
                for (int i = 0; i < t->struct_t.field_count; i++) {
                    c->struct_t.field_names[i] = strdup(t->struct_t.field_names[i]);
                    c->struct_t.field_types[i] = type_clone(t->struct_t.field_types[i]);
                }
            } else {
                c->struct_t.field_names = NULL;
                c->struct_t.field_types = NULL;
            }
            break;
        case TYPE_FN:
            c->fn.ret_type = type_clone(t->fn.ret_type);
            c->fn.param_count = t->fn.param_count;
            if (t->fn.param_count > 0) {
                c->fn.param_types = malloc(sizeof(Type*) * t->fn.param_count);
                for (int i = 0; i < t->fn.param_count; i++) {
                    c->fn.param_types[i] = type_clone(t->fn.param_types[i]);
                }
            } else {
                c->fn.param_types = NULL;
            }
            break;
        default:
            break;
    }
    return c;
}

void type_free(Type *t) {
    if (!t) return;
    switch (t->kind) {
        case TYPE_PTR:
            type_free(t->ptr_to);
            break;
        case TYPE_ARRAY:
        case TYPE_SLICE:
            type_free(t->array.elem_type);
            break;
        case TYPE_STRUCT:
            free(t->struct_t.name);
            for (int i = 0; i < t->struct_t.field_count; i++) {
                free(t->struct_t.field_names[i]);
                type_free(t->struct_t.field_types[i]);
            }
            free(t->struct_t.field_names);
            free(t->struct_t.field_types);
            break;
        case TYPE_ENUM:
            free(t->enum_t.name);
            for (int i = 0; i < t->enum_t.variant_count; i++) {
                free(t->enum_t.variant_names[i]);
            }
            free(t->enum_t.variant_names);
            free(t->enum_t.variant_values);
            break;
        case TYPE_FN:
            type_free(t->fn.ret_type);
            for (int i = 0; i < t->fn.param_count; i++) {
                type_free(t->fn.param_types[i]);
            }
            free(t->fn.param_types);
            break;
        default:
            break;
    }
    free(t);
}

char *type_to_string(Type *t) {
    if (!t) return strdup("unknown");
    char buf[256];
    switch (t->kind) {
        case TYPE_VOID: return strdup("void");
        case TYPE_BOOL: return strdup("bool");
        case TYPE_I8: return strdup("i8");
        case TYPE_I16: return strdup("i16");
        case TYPE_I32: return strdup("i32");
        case TYPE_I64: return strdup("i64");
        case TYPE_U8: return strdup("u8");
        case TYPE_U16: return strdup("u16");
        case TYPE_U32: return strdup("u32");
        case TYPE_U64: return strdup("u64");
        case TYPE_F32: return strdup("f32");
        case TYPE_F64: return strdup("f64");
        case TYPE_PTR: {
            char *inner = type_to_string(t->ptr_to);
            snprintf(buf, sizeof(buf), "ptr<%s>", inner);
            free(inner);
            return strdup(buf);
        }
        case TYPE_ARRAY: {
            char *inner = type_to_string(t->array.elem_type);
            snprintf(buf, sizeof(buf), "[%s; %d]", inner, t->array.size);
            free(inner);
            return strdup(buf);
        }
        case TYPE_SLICE: {
            char *inner = type_to_string(t->array.elem_type);
            snprintf(buf, sizeof(buf), "[%s]", inner);
            free(inner);
            return strdup(buf);
        }
        case TYPE_STRUCT:
            return strdup(t->struct_t.name ? t->struct_t.name : "struct");
        case TYPE_ENUM:
            return strdup(t->enum_t.name ? t->enum_t.name : "enum");
        case TYPE_FN:
            return strdup("fn");
        case TYPE_UNKNOWN:
            return strdup("unknown");
    }
    return strdup("?");
}

bool type_equals(Type *a, Type *b) {
    if (!a || !b) return false;
    if (a->kind != b->kind) return false;
    switch (a->kind) {
        case TYPE_PTR:
            return type_equals(a->ptr_to, b->ptr_to);
        case TYPE_ARRAY:
            return a->array.size == b->array.size &&
                   type_equals(a->array.elem_type, b->array.elem_type);
        case TYPE_SLICE:
            return type_equals(a->array.elem_type, b->array.elem_type);
        case TYPE_STRUCT:
            return a->struct_t.name && b->struct_t.name &&
                   strcmp(a->struct_t.name, b->struct_t.name) == 0;
        case TYPE_ENUM:
            return a->enum_t.name && b->enum_t.name &&
                   strcmp(a->enum_t.name, b->enum_t.name) == 0;
        default:
            return true;
    }
}

// AST utilities
ASTNode *ast_new(NodeKind kind, int line, int column) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->kind = kind;
    node->line = line;
    node->column = column;
    return node;
}

void ast_free(ASTNode *node) {
    if (!node) return;

    switch (node->kind) {
        case NODE_PROGRAM:
            for (int i = 0; i < node->program.decl_count; i++) {
                ast_free(node->program.decls[i]);
            }
            free(node->program.decls);
            break;
        case NODE_FN_DECL:
            free(node->fn_decl.name);
            for (int i = 0; i < node->fn_decl.param_count; i++) {
                ast_free(node->fn_decl.params[i]);
            }
            free(node->fn_decl.params);
            type_free(node->fn_decl.ret_type);
            ast_free(node->fn_decl.body);
            break;
        case NODE_STRUCT_DECL:
            free(node->struct_decl.name);
            for (int i = 0; i < node->struct_decl.field_count; i++) {
                free(node->struct_decl.field_names[i]);
                type_free(node->struct_decl.field_types[i]);
            }
            free(node->struct_decl.field_names);
            free(node->struct_decl.field_types);
            break;
        case NODE_ENUM_DECL:
            free(node->enum_decl.name);
            for (int i = 0; i < node->enum_decl.variant_count; i++) {
                free(node->enum_decl.variant_names[i]);
            }
            free(node->enum_decl.variant_names);
            free(node->enum_decl.variant_values);
            break;
        case NODE_VAR_DECL:
            free(node->var_decl.name);
            type_free(node->var_decl.var_type);
            ast_free(node->var_decl.init);
            break;
        case NODE_PARAM:
            free(node->param.name);
            type_free(node->param.param_type);
            break;
        case NODE_BLOCK:
            for (int i = 0; i < node->block.stmt_count; i++) {
                ast_free(node->block.stmts[i]);
            }
            free(node->block.stmts);
            break;
        case NODE_RETURN:
            ast_free(node->ret.value);
            break;
        case NODE_IF:
            ast_free(node->if_stmt.cond);
            ast_free(node->if_stmt.then_block);
            for (int i = 0; i < node->if_stmt.elif_count; i++) {
                ast_free(node->if_stmt.elif_conds[i]);
                ast_free(node->if_stmt.elif_blocks[i]);
            }
            free(node->if_stmt.elif_conds);
            free(node->if_stmt.elif_blocks);
            ast_free(node->if_stmt.else_block);
            break;
        case NODE_WHILE:
            ast_free(node->while_stmt.cond);
            ast_free(node->while_stmt.body);
            break;
        case NODE_FOR:
            free(node->for_stmt.var_name);
            ast_free(node->for_stmt.start);
            ast_free(node->for_stmt.end);
            ast_free(node->for_stmt.body);
            break;
        case NODE_ASSIGN:
            ast_free(node->assign.target);
            ast_free(node->assign.value);
            break;
        case NODE_BINARY:
            ast_free(node->binary.left);
            ast_free(node->binary.right);
            break;
        case NODE_UNARY:
            ast_free(node->unary.operand);
            break;
        case NODE_CALL:
            ast_free(node->call.callee);
            for (int i = 0; i < node->call.arg_count; i++) {
                ast_free(node->call.args[i]);
            }
            free(node->call.args);
            break;
        case NODE_MEMBER:
            ast_free(node->member.object);
            free(node->member.member);
            break;
        case NODE_INDEX:
            ast_free(node->index.object);
            ast_free(node->index.index);
            break;
        case NODE_LITERAL_STRING:
            free(node->string_val);
            break;
        case NODE_IDENT:
            free(node->ident);
            break;
        case NODE_STRUCT_INIT:
            free(node->struct_init.struct_name);
            for (int i = 0; i < node->struct_init.field_count; i++) {
                free(node->struct_init.field_names[i]);
                ast_free(node->struct_init.field_values[i]);
            }
            free(node->struct_init.field_names);
            free(node->struct_init.field_values);
            break;
        case NODE_ARRAY_INIT:
            for (int i = 0; i < node->array_init.elem_count; i++) {
                ast_free(node->array_init.elements[i]);
            }
            free(node->array_init.elements);
            break;
        case NODE_USE:
            free(node->use.path);
            free(node->use.alias);
            break;
        case NODE_ENUM_VARIANT:
            free(node->enum_variant.enum_name);
            free(node->enum_variant.variant_name);
            break;
        case NODE_EXTERN:
            free(node->extern_decl.abi);
            for (int i = 0; i < node->extern_decl.fn_count; i++) {
                ast_free(node->extern_decl.fn_decls[i]);
            }
            free(node->extern_decl.fn_decls);
            break;
        case NODE_EXPR_STMT:
            ast_free(node->expr_stmt.expr);
            break;
        default:
            break;
    }

    type_free(node->type);
    free(node);
}

// Parser implementation
static void advance(Parser *p);
static void consume(Parser *p, TokenType type, const char *msg);
static bool check(Parser *p, TokenType type);
static bool match(Parser *p, TokenType type);
static void skip_newlines(Parser *p);
static void error_at(Parser *p, Token *token, const char *msg);

static ASTNode *parse_decl(Parser *p);
static ASTNode *parse_fn_decl(Parser *p);
static ASTNode *parse_struct_decl(Parser *p);
static ASTNode *parse_enum_decl(Parser *p);
static ASTNode *parse_var_decl(Parser *p);
static ASTNode *parse_use(Parser *p);
static ASTNode *parse_extern(Parser *p);
static ASTNode *parse_stmt(Parser *p);
static ASTNode *parse_block(Parser *p);
static ASTNode *parse_if(Parser *p);
static ASTNode *parse_while(Parser *p);
static ASTNode *parse_for(Parser *p);
static ASTNode *parse_return(Parser *p);
static ASTNode *parse_expr_stmt(Parser *p);
static ASTNode *parse_expr(Parser *p);
static ASTNode *parse_assignment(Parser *p);
static ASTNode *parse_or(Parser *p);
static ASTNode *parse_and(Parser *p);
static ASTNode *parse_equality(Parser *p);
static ASTNode *parse_comparison(Parser *p);
static ASTNode *parse_bitwise_or(Parser *p);
static ASTNode *parse_bitwise_xor(Parser *p);
static ASTNode *parse_bitwise_and(Parser *p);
static ASTNode *parse_shift(Parser *p);
static ASTNode *parse_term(Parser *p);
static ASTNode *parse_factor(Parser *p);
static ASTNode *parse_unary(Parser *p);
static ASTNode *parse_postfix(Parser *p);
static ASTNode *parse_primary(Parser *p);
static Type *parse_type(Parser *p);

void parser_init(Parser *parser, Lexer *lexer) {
    parser->lexer = lexer;
    parser->had_error = false;
    parser->panic_mode = false;
    parser->error_msg = NULL;
    advance(parser);
}

// Print the source line with error highlighting
static void print_error_context(Lexer *lexer, int line, int column, int token_len) {
    const char *line_start = lexer_get_line(lexer, line);
    int line_len = lexer_get_line_length(lexer, line);

    if (!line_start || line_len == 0) return;

    // Print line number and source line
    fprintf(stderr, "  %4d | ", line);
    for (int i = 0; i < line_len; i++) {
        // Replace tabs with spaces for consistent alignment
        if (line_start[i] == '\t') {
            fprintf(stderr, "    ");
        } else {
            fputc(line_start[i], stderr);
        }
    }
    fprintf(stderr, "\n");

    // Print caret pointing to error location
    fprintf(stderr, "       | ");
    for (int i = 1; i < column; i++) {
        // Match tab handling
        if (i <= line_len && line_start[i-1] == '\t') {
            fprintf(stderr, "    ");
        } else {
            fputc(' ', stderr);
        }
    }

    // Print caret(s) under the token
    fprintf(stderr, "^");
    for (int i = 1; i < token_len && (column + i - 1) < line_len; i++) {
        fprintf(stderr, "~");
    }
    fprintf(stderr, "\n");
}

// Get a hint for common errors
static const char *get_error_hint(const char *msg, Token *token) {
    // Missing 'end' keyword hints
    if (strstr(msg, "Expected 'end'")) {
        return "Hint: Every 'do' block must be closed with 'end'";
    }

    // Missing closing bracket hints
    if (strstr(msg, "Expected ']'")) {
        return "Hint: Arrays must have matching brackets: [1, 2, 3]";
    }
    if (strstr(msg, "Expected ')'")) {
        return "Hint: Function calls and expressions need matching parentheses";
    }
    if (strstr(msg, "Expected '}'")) {
        return "Hint: Struct literals need matching braces: Point { x = 1, y = 2 }";
    }

    // Type annotation hints
    if (strstr(msg, "Expected type")) {
        return "Hint: Use type annotations like :: i64, :: bool, :: ptr<u8>";
    }

    // Expression hints
    if (strstr(msg, "Expected expression")) {
        return "Hint: An expression is a value like: 42, x + y, fn_call(), true";
    }

    // Missing 'do' hints
    if (strstr(msg, "Expected 'do'")) {
        return "Hint: Control structures use 'do' to start their body: if x > 0 do ... end";
    }

    return NULL;
}

static void error_at(Parser *p, Token *token, const char *msg) {
    if (p->panic_mode) return;
    p->panic_mode = true;
    p->had_error = true;

    // Print header
    fprintf(stderr, "\n\033[1;31mError\033[0m at line %d, column %d", token->line, token->column);
    if (token->type == TOK_EOF) {
        fprintf(stderr, " (end of file)");
    } else if (token->type != TOK_ERROR) {
        fprintf(stderr, " near '\033[1m%.*s\033[0m'", (int)token->length, token->start);
    }
    fprintf(stderr, "\n");

    // Print source context
    print_error_context(p->lexer, token->line, token->column,
        token->type == TOK_EOF ? 1 : (int)token->length);

    // Print error message
    fprintf(stderr, "\033[1;31m%s\033[0m\n", msg);

    // Print hint if available
    const char *hint = get_error_hint(msg, token);
    if (hint) {
        fprintf(stderr, "\033[36m%s\033[0m\n", hint);
    }
    fprintf(stderr, "\n");
}

static void advance(Parser *p) {
    p->previous = p->current;
    for (;;) {
        p->current = lexer_next(p->lexer);
        if (p->current.type != TOK_ERROR) break;
        error_at(p, &p->current, p->current.start);
    }
}

static void consume(Parser *p, TokenType type, const char *msg) {
    if (p->current.type == type) {
        advance(p);
        return;
    }
    error_at(p, &p->current, msg);
}

static bool check(Parser *p, TokenType type) {
    return p->current.type == type;
}

static bool match(Parser *p, TokenType type) {
    if (!check(p, type)) return false;
    advance(p);
    return true;
}

static void skip_newlines(Parser *p) {
    while (match(p, TOK_NEWLINE)) {}
}

ASTNode *parser_parse(Parser *parser) {
    ASTNode *program = ast_new(NODE_PROGRAM, 1, 1);
    program->program.decls = NULL;
    program->program.decl_count = 0;

    skip_newlines(parser);

    while (!check(parser, TOK_EOF)) {
        ASTNode *decl = parse_decl(parser);
        if (decl) {
            ENSURE_CAPACITY(program->program.decls, program->program.decl_count,
                program->program.decl_capacity, ASTNode*);
            program->program.decls[program->program.decl_count++] = decl;
        }
        skip_newlines(parser);
    }

    return program;
}

static ASTNode *parse_decl(Parser *p) {
    skip_newlines(p);

    if (match(p, TOK_DIR_USE)) {
        return parse_use(p);
    }
    if (match(p, TOK_DIR_EXTERN)) {
        return parse_extern(p);
    }
    if (check(p, TOK_FN)) {
        return parse_fn_decl(p);
    }
    if (check(p, TOK_STRUCT)) {
        return parse_struct_decl(p);
    }
    if (check(p, TOK_ENUM)) {
        return parse_enum_decl(p);
    }
    if (check(p, TOK_LET) || check(p, TOK_MUT) || check(p, TOK_CONST)) {
        return parse_var_decl(p);
    }

    return parse_stmt(p);
}

static ASTNode *parse_use(Parser *p) {
    ASTNode *node = ast_new(NODE_USE, p->previous.line, p->previous.column);

    consume(p, TOK_STRING_LIT, "Expected path string after @use.");
    node->use.path = str_dup(p->previous.start + 1, p->previous.length - 2);
    node->use.alias = NULL;

    if (match(p, TOK_AS)) {
        consume(p, TOK_IDENT, "Expected alias name after 'as'.");
        node->use.alias = str_dup(p->previous.start, p->previous.length);
    }

    return node;
}

static ASTNode *parse_extern(Parser *p) {
    ASTNode *node = ast_new(NODE_EXTERN, p->previous.line, p->previous.column);

    consume(p, TOK_STRING_LIT, "Expected ABI string after @extern.");
    node->extern_decl.abi = str_dup(p->previous.start + 1, p->previous.length - 2);
    node->extern_decl.fn_decls = NULL;
    node->extern_decl.fn_count = 0;

    consume(p, TOK_DO, "Expected 'do' after @extern ABI.");
    skip_newlines(p);

    while (!check(p, TOK_END) && !check(p, TOK_EOF)) {
        ASTNode *fn = parse_fn_decl(p);
        if (fn) {
            fn->fn_decl.is_extern = true;
            node->extern_decl.fn_count++;
            node->extern_decl.fn_decls = safe_realloc(node->extern_decl.fn_decls,
                sizeof(ASTNode*) * node->extern_decl.fn_count);
            node->extern_decl.fn_decls[node->extern_decl.fn_count - 1] = fn;
        }
        skip_newlines(p);
    }

    consume(p, TOK_END, "Expected 'end' after extern block.");
    return node;
}

static ASTNode *parse_fn_decl(Parser *p) {
    consume(p, TOK_FN, "Expected 'fn'.");
    ASTNode *node = ast_new(NODE_FN_DECL, p->previous.line, p->previous.column);

    consume(p, TOK_IDENT, "Expected function name.");
    node->fn_decl.name = str_dup(p->previous.start, p->previous.length);
    node->fn_decl.params = NULL;
    node->fn_decl.param_count = 0;
    node->fn_decl.is_extern = false;

    consume(p, TOK_LPAREN, "Expected '(' after function name.");

    if (!check(p, TOK_RPAREN)) {
        do {
            ASTNode *param = ast_new(NODE_PARAM, p->current.line, p->current.column);
            consume(p, TOK_IDENT, "Expected parameter name.");
            param->param.name = str_dup(p->previous.start, p->previous.length);

            consume(p, TOK_COLONCOLON, "Expected '::' before parameter type.");
            param->param.param_type = parse_type(p);

            ENSURE_CAPACITY(node->fn_decl.params, node->fn_decl.param_count,
                node->fn_decl.param_capacity, ASTNode*);
            node->fn_decl.params[node->fn_decl.param_count++] = param;
        } while (match(p, TOK_COMMA));
    }

    consume(p, TOK_RPAREN, "Expected ')' after parameters.");

    // Return type
    if (match(p, TOK_ARROW)) {
        node->fn_decl.ret_type = parse_type(p);
    } else {
        node->fn_decl.ret_type = type_new(TYPE_VOID);
    }

    // Body (optional for extern)
    if (match(p, TOK_DO)) {
        node->fn_decl.body = parse_block(p);
    } else {
        node->fn_decl.body = NULL;
    }

    return node;
}

static ASTNode *parse_struct_decl(Parser *p) {
    consume(p, TOK_STRUCT, "Expected 'struct'.");
    ASTNode *node = ast_new(NODE_STRUCT_DECL, p->previous.line, p->previous.column);

    consume(p, TOK_IDENT, "Expected struct name.");
    node->struct_decl.name = str_dup(p->previous.start, p->previous.length);
    node->struct_decl.field_names = NULL;
    node->struct_decl.field_types = NULL;
    node->struct_decl.field_count = 0;

    consume(p, TOK_DO, "Expected 'do' after struct name.");
    skip_newlines(p);

    while (!check(p, TOK_END) && !check(p, TOK_EOF)) {
        consume(p, TOK_IDENT, "Expected field name.");
        char *field_name = str_dup(p->previous.start, p->previous.length);

        consume(p, TOK_COLONCOLON, "Expected '::' after field name.");
        Type *field_type = parse_type(p);

        node->struct_decl.field_count++;
        node->struct_decl.field_names = safe_realloc(node->struct_decl.field_names,
            sizeof(char*) * node->struct_decl.field_count);
        node->struct_decl.field_types = safe_realloc(node->struct_decl.field_types,
            sizeof(Type*) * node->struct_decl.field_count);
        node->struct_decl.field_names[node->struct_decl.field_count - 1] = field_name;
        node->struct_decl.field_types[node->struct_decl.field_count - 1] = field_type;

        skip_newlines(p);
    }

    consume(p, TOK_END, "Expected 'end' after struct body.");
    return node;
}

static ASTNode *parse_enum_decl(Parser *p) {
    consume(p, TOK_ENUM, "Expected 'enum'.");
    ASTNode *node = ast_new(NODE_ENUM_DECL, p->previous.line, p->previous.column);

    consume(p, TOK_IDENT, "Expected enum name.");
    node->enum_decl.name = str_dup(p->previous.start, p->previous.length);
    node->enum_decl.variant_names = NULL;
    node->enum_decl.variant_values = NULL;
    node->enum_decl.variant_count = 0;

    consume(p, TOK_DO, "Expected 'do' after enum name.");
    skip_newlines(p);

    int64_t next_value = 0;  // Auto-increment starting at 0

    while (!check(p, TOK_END) && !check(p, TOK_EOF)) {
        consume(p, TOK_IDENT, "Expected variant name.");
        char *variant_name = str_dup(p->previous.start, p->previous.length);

        int64_t value = next_value;
        // Optional explicit value: Variant = 42
        if (match(p, TOK_EQ)) {
            consume(p, TOK_INT_LIT, "Expected integer value for enum variant.");
            value = p->previous.int_value;
        }
        next_value = value + 1;

        node->enum_decl.variant_count++;
        node->enum_decl.variant_names = safe_realloc(node->enum_decl.variant_names,
            sizeof(char*) * node->enum_decl.variant_count);
        node->enum_decl.variant_values = safe_realloc(node->enum_decl.variant_values,
            sizeof(int64_t) * node->enum_decl.variant_count);
        node->enum_decl.variant_names[node->enum_decl.variant_count - 1] = variant_name;
        node->enum_decl.variant_values[node->enum_decl.variant_count - 1] = value;

        skip_newlines(p);
    }

    consume(p, TOK_END, "Expected 'end' after enum body.");
    return node;
}

static ASTNode *parse_var_decl(Parser *p) {
    bool is_mut = match(p, TOK_MUT);
    bool is_const = false;
    if (!is_mut) {
        is_const = match(p, TOK_CONST);
        if (!is_const) {
            consume(p, TOK_LET, "Expected 'let', 'mut', or 'const'.");
        }
    }

    ASTNode *node = ast_new(NODE_VAR_DECL, p->previous.line, p->previous.column);
    node->var_decl.is_mut = is_mut;
    node->var_decl.is_const = is_const;

    consume(p, TOK_IDENT, "Expected variable name.");
    node->var_decl.name = str_dup(p->previous.start, p->previous.length);

    // Optional type annotation
    if (match(p, TOK_COLONCOLON)) {
        node->var_decl.var_type = parse_type(p);
    } else {
        node->var_decl.var_type = NULL; // infer
    }

    consume(p, TOK_EQ, "Expected '=' in variable declaration.");
    node->var_decl.init = parse_expr(p);

    return node;
}

static Type *parse_type(Parser *p) {
    if (match(p, TOK_VOID)) return type_new(TYPE_VOID);
    if (match(p, TOK_BOOL)) return type_new(TYPE_BOOL);
    if (match(p, TOK_I8)) return type_new(TYPE_I8);
    if (match(p, TOK_I16)) return type_new(TYPE_I16);
    if (match(p, TOK_I32)) return type_new(TYPE_I32);
    if (match(p, TOK_I64)) return type_new(TYPE_I64);
    if (match(p, TOK_U8)) return type_new(TYPE_U8);
    if (match(p, TOK_U16)) return type_new(TYPE_U16);
    if (match(p, TOK_U32)) return type_new(TYPE_U32);
    if (match(p, TOK_U64)) return type_new(TYPE_U64);
    if (match(p, TOK_F32)) return type_new(TYPE_F32);
    if (match(p, TOK_F64)) return type_new(TYPE_F64);

    // ptr<T>
    if (match(p, TOK_PTR)) {
        Type *t = type_new(TYPE_PTR);
        consume(p, TOK_LT, "Expected '<' after 'ptr'.");
        t->ptr_to = parse_type(p);
        consume(p, TOK_GT, "Expected '>' after pointer type.");
        return t;
    }

    // [T] or [T; N]
    if (match(p, TOK_LBRACKET)) {
        Type *elem = parse_type(p);
        if (match(p, TOK_SEMICOLON)) {
            consume(p, TOK_INT_LIT, "Expected array size.");
            if (p->previous.int_value < 0 || p->previous.int_value > INT32_MAX) {
                error_at(p, &p->previous, "Array size out of range.");
                type_free(elem);
                return type_new(TYPE_UNKNOWN);
            }
            int size = (int)p->previous.int_value;
            consume(p, TOK_RBRACKET, "Expected ']'.");
            Type *t = type_new(TYPE_ARRAY);
            t->array.elem_type = elem;
            t->array.size = size;
            return t;
        }
        consume(p, TOK_RBRACKET, "Expected ']'.");
        Type *t = type_new(TYPE_SLICE);
        t->array.elem_type = elem;
        t->array.size = -1;
        return t;
    }

    // Named type (struct)
    if (match(p, TOK_IDENT)) {
        Type *t = type_new(TYPE_STRUCT);
        t->struct_t.name = str_dup(p->previous.start, p->previous.length);
        t->struct_t.field_count = 0;
        t->struct_t.field_names = NULL;
        t->struct_t.field_types = NULL;
        return t;
    }

    error_at(p, &p->current, "Expected type.");
    return type_new(TYPE_UNKNOWN);
}

static ASTNode *parse_block(Parser *p) {
    ASTNode *node = ast_new(NODE_BLOCK, p->previous.line, p->previous.column);
    node->block.stmts = NULL;
    node->block.stmt_count = 0;

    skip_newlines(p);

    while (!check(p, TOK_END) && !check(p, TOK_ELIF) &&
           !check(p, TOK_ELSE) && !check(p, TOK_EOF)) {
        ASTNode *stmt = parse_stmt(p);
        if (stmt) {
            ENSURE_CAPACITY(node->block.stmts, node->block.stmt_count,
                node->block.stmt_capacity, ASTNode*);
            node->block.stmts[node->block.stmt_count++] = stmt;
        }
        skip_newlines(p);
    }

    if (check(p, TOK_END)) {
        advance(p);
    }

    return node;
}

static ASTNode *parse_stmt(Parser *p) {
    skip_newlines(p);

    if (check(p, TOK_LET) || check(p, TOK_MUT) || check(p, TOK_CONST)) {
        return parse_var_decl(p);
    }
    if (check(p, TOK_RET)) {
        return parse_return(p);
    }
    if (match(p, TOK_BREAK)) {
        return ast_new(NODE_BREAK, p->previous.line, p->previous.column);
    }
    if (match(p, TOK_CONTINUE)) {
        return ast_new(NODE_CONTINUE, p->previous.line, p->previous.column);
    }
    if (check(p, TOK_IF)) {
        return parse_if(p);
    }
    if (check(p, TOK_WHILE)) {
        return parse_while(p);
    }
    if (check(p, TOK_FOR)) {
        return parse_for(p);
    }

    return parse_expr_stmt(p);
}

static ASTNode *parse_return(Parser *p) {
    consume(p, TOK_RET, "Expected 'ret'.");
    ASTNode *node = ast_new(NODE_RETURN, p->previous.line, p->previous.column);

    if (!check(p, TOK_NEWLINE) && !check(p, TOK_END) && !check(p, TOK_EOF)) {
        node->ret.value = parse_expr(p);
    } else {
        node->ret.value = NULL;
    }

    return node;
}

static ASTNode *parse_if(Parser *p) {
    consume(p, TOK_IF, "Expected 'if'.");
    ASTNode *node = ast_new(NODE_IF, p->previous.line, p->previous.column);

    node->if_stmt.cond = parse_expr(p);
    consume(p, TOK_DO, "Expected 'do' after if condition.");

    // Parse then block but stop at elif/else/end
    node->if_stmt.then_block = ast_new(NODE_BLOCK, p->previous.line, p->previous.column);
    node->if_stmt.then_block->block.stmts = NULL;
    node->if_stmt.then_block->block.stmt_count = 0;

    skip_newlines(p);
    while (!check(p, TOK_END) && !check(p, TOK_ELIF) &&
           !check(p, TOK_ELSE) && !check(p, TOK_EOF)) {
        ASTNode *stmt = parse_stmt(p);
        if (stmt) {
            ENSURE_CAPACITY(node->if_stmt.then_block->block.stmts,
                node->if_stmt.then_block->block.stmt_count,
                node->if_stmt.then_block->block.stmt_capacity, ASTNode*);
            node->if_stmt.then_block->block.stmts[
                node->if_stmt.then_block->block.stmt_count++] = stmt;
        }
        skip_newlines(p);
    }

    // elif chains
    node->if_stmt.elif_conds = NULL;
    node->if_stmt.elif_blocks = NULL;
    node->if_stmt.elif_count = 0;

    while (match(p, TOK_ELIF)) {
        ASTNode *elif_cond = parse_expr(p);
        consume(p, TOK_DO, "Expected 'do' after elif condition.");

        ASTNode *elif_block = ast_new(NODE_BLOCK, p->previous.line, p->previous.column);
        elif_block->block.stmts = NULL;
        elif_block->block.stmt_count = 0;

        skip_newlines(p);
        while (!check(p, TOK_END) && !check(p, TOK_ELIF) &&
               !check(p, TOK_ELSE) && !check(p, TOK_EOF)) {
            ASTNode *stmt = parse_stmt(p);
            if (stmt) {
                ENSURE_CAPACITY(elif_block->block.stmts, elif_block->block.stmt_count,
                    elif_block->block.stmt_capacity, ASTNode*);
                elif_block->block.stmts[elif_block->block.stmt_count++] = stmt;
            }
            skip_newlines(p);
        }

        node->if_stmt.elif_count++;
        node->if_stmt.elif_conds = safe_realloc(node->if_stmt.elif_conds,
            sizeof(ASTNode*) * node->if_stmt.elif_count);
        node->if_stmt.elif_blocks = safe_realloc(node->if_stmt.elif_blocks,
            sizeof(ASTNode*) * node->if_stmt.elif_count);
        node->if_stmt.elif_conds[node->if_stmt.elif_count - 1] = elif_cond;
        node->if_stmt.elif_blocks[node->if_stmt.elif_count - 1] = elif_block;
    }

    // else block
    node->if_stmt.else_block = NULL;
    if (match(p, TOK_ELSE)) {
        consume(p, TOK_DO, "Expected 'do' after else.");
        node->if_stmt.else_block = ast_new(NODE_BLOCK, p->previous.line, p->previous.column);
        node->if_stmt.else_block->block.stmts = NULL;
        node->if_stmt.else_block->block.stmt_count = 0;

        skip_newlines(p);
        while (!check(p, TOK_END) && !check(p, TOK_EOF)) {
            ASTNode *stmt = parse_stmt(p);
            if (stmt) {
                ENSURE_CAPACITY(node->if_stmt.else_block->block.stmts,
                    node->if_stmt.else_block->block.stmt_count,
                    node->if_stmt.else_block->block.stmt_capacity, ASTNode*);
                node->if_stmt.else_block->block.stmts[
                    node->if_stmt.else_block->block.stmt_count++] = stmt;
            }
            skip_newlines(p);
        }
    }

    consume(p, TOK_END, "Expected 'end' after if statement.");
    return node;
}

static ASTNode *parse_while(Parser *p) {
    consume(p, TOK_WHILE, "Expected 'while'.");
    ASTNode *node = ast_new(NODE_WHILE, p->previous.line, p->previous.column);

    node->while_stmt.cond = parse_expr(p);
    consume(p, TOK_DO, "Expected 'do' after while condition.");
    node->while_stmt.body = parse_block(p);

    return node;
}

static ASTNode *parse_for(Parser *p) {
    consume(p, TOK_FOR, "Expected 'for'.");
    ASTNode *node = ast_new(NODE_FOR, p->previous.line, p->previous.column);

    consume(p, TOK_IDENT, "Expected loop variable.");
    node->for_stmt.var_name = str_dup(p->previous.start, p->previous.length);

    consume(p, TOK_IN, "Expected 'in' in for loop.");

    node->for_stmt.start = parse_expr(p);
    consume(p, TOK_DOTDOT, "Expected '..' in range.");
    node->for_stmt.end = parse_expr(p);

    consume(p, TOK_DO, "Expected 'do' after for range.");
    node->for_stmt.body = parse_block(p);

    return node;
}

static ASTNode *parse_expr_stmt(Parser *p) {
    ASTNode *expr = parse_expr(p);
    if (!expr) return NULL;

    ASTNode *node = ast_new(NODE_EXPR_STMT, expr->line, expr->column);
    node->expr_stmt.expr = expr;
    return node;
}

static ASTNode *parse_expr(Parser *p) {
    return parse_assignment(p);
}

static ASTNode *parse_assignment(Parser *p) {
    ASTNode *expr = parse_or(p);

    if (match(p, TOK_EQ)) {
        ASTNode *value = parse_assignment(p);
        ASTNode *node = ast_new(NODE_ASSIGN, expr->line, expr->column);
        node->assign.target = expr;
        node->assign.value = value;
        return node;
    }

    return expr;
}

static ASTNode *parse_or(Parser *p) {
    ASTNode *left = parse_and(p);

    while (match(p, TOK_OR)) {
        ASTNode *right = parse_and(p);
        ASTNode *node = ast_new(NODE_BINARY, left->line, left->column);
        node->binary.op = BIN_OR;
        node->binary.left = left;
        node->binary.right = right;
        left = node;
    }

    return left;
}

static ASTNode *parse_and(Parser *p) {
    ASTNode *left = parse_equality(p);

    while (match(p, TOK_AND)) {
        ASTNode *right = parse_equality(p);
        ASTNode *node = ast_new(NODE_BINARY, left->line, left->column);
        node->binary.op = BIN_AND;
        node->binary.left = left;
        node->binary.right = right;
        left = node;
    }

    return left;
}

static ASTNode *parse_equality(Parser *p) {
    ASTNode *left = parse_comparison(p);

    while (match(p, TOK_EQEQ) || match(p, TOK_NE)) {
        BinaryOp op = p->previous.type == TOK_EQEQ ? BIN_EQ : BIN_NE;
        ASTNode *right = parse_comparison(p);
        ASTNode *node = ast_new(NODE_BINARY, left->line, left->column);
        node->binary.op = op;
        node->binary.left = left;
        node->binary.right = right;
        left = node;
    }

    return left;
}

static ASTNode *parse_comparison(Parser *p) {
    ASTNode *left = parse_bitwise_or(p);

    while (match(p, TOK_LT) || match(p, TOK_LE) ||
           match(p, TOK_GT) || match(p, TOK_GE)) {
        BinaryOp op;
        switch (p->previous.type) {
            case TOK_LT: op = BIN_LT; break;
            case TOK_LE: op = BIN_LE; break;
            case TOK_GT: op = BIN_GT; break;
            case TOK_GE: op = BIN_GE; break;
            default: op = BIN_LT;
        }
        ASTNode *right = parse_bitwise_or(p);
        ASTNode *node = ast_new(NODE_BINARY, left->line, left->column);
        node->binary.op = op;
        node->binary.left = left;
        node->binary.right = right;
        left = node;
    }

    return left;
}

static ASTNode *parse_bitwise_or(Parser *p) {
    ASTNode *left = parse_bitwise_xor(p);

    while (match(p, TOK_PIPE)) {
        ASTNode *right = parse_bitwise_xor(p);
        ASTNode *node = ast_new(NODE_BINARY, left->line, left->column);
        node->binary.op = BIN_BOR;
        node->binary.left = left;
        node->binary.right = right;
        left = node;
    }

    return left;
}

static ASTNode *parse_bitwise_xor(Parser *p) {
    ASTNode *left = parse_bitwise_and(p);

    while (match(p, TOK_CARET)) {
        ASTNode *right = parse_bitwise_and(p);
        ASTNode *node = ast_new(NODE_BINARY, left->line, left->column);
        node->binary.op = BIN_BXOR;
        node->binary.left = left;
        node->binary.right = right;
        left = node;
    }

    return left;
}

static ASTNode *parse_bitwise_and(Parser *p) {
    ASTNode *left = parse_shift(p);

    while (match(p, TOK_AMP)) {
        ASTNode *right = parse_shift(p);
        ASTNode *node = ast_new(NODE_BINARY, left->line, left->column);
        node->binary.op = BIN_BAND;
        node->binary.left = left;
        node->binary.right = right;
        left = node;
    }

    return left;
}

static ASTNode *parse_shift(Parser *p) {
    ASTNode *left = parse_term(p);

    while (match(p, TOK_LSHIFT) || match(p, TOK_RSHIFT)) {
        BinaryOp op = p->previous.type == TOK_LSHIFT ? BIN_LSHIFT : BIN_RSHIFT;
        ASTNode *right = parse_term(p);
        ASTNode *node = ast_new(NODE_BINARY, left->line, left->column);
        node->binary.op = op;
        node->binary.left = left;
        node->binary.right = right;
        left = node;
    }

    return left;
}

static ASTNode *parse_term(Parser *p) {
    ASTNode *left = parse_factor(p);

    while (match(p, TOK_PLUS) || match(p, TOK_MINUS)) {
        BinaryOp op = p->previous.type == TOK_PLUS ? BIN_ADD : BIN_SUB;
        ASTNode *right = parse_factor(p);
        ASTNode *node = ast_new(NODE_BINARY, left->line, left->column);
        node->binary.op = op;
        node->binary.left = left;
        node->binary.right = right;
        left = node;
    }

    return left;
}

static ASTNode *parse_factor(Parser *p) {
    ASTNode *left = parse_unary(p);

    while (match(p, TOK_STAR) || match(p, TOK_SLASH) || match(p, TOK_PERCENT)) {
        BinaryOp op;
        switch (p->previous.type) {
            case TOK_STAR: op = BIN_MUL; break;
            case TOK_SLASH: op = BIN_DIV; break;
            case TOK_PERCENT: op = BIN_MOD; break;
            default: op = BIN_MUL;
        }
        ASTNode *right = parse_unary(p);
        ASTNode *node = ast_new(NODE_BINARY, left->line, left->column);
        node->binary.op = op;
        node->binary.left = left;
        node->binary.right = right;
        left = node;
    }

    return left;
}

static ASTNode *parse_unary(Parser *p) {
    if (match(p, TOK_MINUS)) {
        ASTNode *operand = parse_unary(p);
        ASTNode *node = ast_new(NODE_UNARY, p->previous.line, p->previous.column);
        node->unary.op = UN_NEG;
        node->unary.operand = operand;
        return node;
    }
    if (match(p, TOK_NOT)) {
        ASTNode *operand = parse_unary(p);
        ASTNode *node = ast_new(NODE_UNARY, p->previous.line, p->previous.column);
        node->unary.op = UN_NOT;
        node->unary.operand = operand;
        return node;
    }
    if (match(p, TOK_TILDE)) {
        ASTNode *operand = parse_unary(p);
        ASTNode *node = ast_new(NODE_UNARY, p->previous.line, p->previous.column);
        node->unary.op = UN_BNOT;
        node->unary.operand = operand;
        return node;
    }
    if (match(p, TOK_AMP)) {
        ASTNode *operand = parse_unary(p);
        ASTNode *node = ast_new(NODE_UNARY, p->previous.line, p->previous.column);
        node->unary.op = UN_ADDR;
        node->unary.operand = operand;
        return node;
    }
    if (match(p, TOK_STAR)) {
        ASTNode *operand = parse_unary(p);
        ASTNode *node = ast_new(NODE_UNARY, p->previous.line, p->previous.column);
        node->unary.op = UN_DEREF;
        node->unary.operand = operand;
        return node;
    }

    return parse_postfix(p);
}

static ASTNode *parse_postfix(Parser *p) {
    ASTNode *left = parse_primary(p);

    for (;;) {
        if (match(p, TOK_LPAREN)) {
            // Function call
            ASTNode *call = ast_new(NODE_CALL, left->line, left->column);
            call->call.callee = left;
            call->call.args = NULL;
            call->call.arg_count = 0;

            if (!check(p, TOK_RPAREN)) {
                do {
                    ASTNode *arg = parse_expr(p);
                    call->call.arg_count++;
                    call->call.args = safe_realloc(call->call.args,
                        sizeof(ASTNode*) * call->call.arg_count);
                    call->call.args[call->call.arg_count - 1] = arg;
                } while (match(p, TOK_COMMA));
            }

            consume(p, TOK_RPAREN, "Expected ')' after arguments.");
            left = call;
        } else if (match(p, TOK_DOT)) {
            // Member access
            consume(p, TOK_IDENT, "Expected member name after '.'");
            ASTNode *member = ast_new(NODE_MEMBER, left->line, left->column);
            member->member.object = left;
            member->member.member = str_dup(p->previous.start, p->previous.length);
            left = member;
        } else if (match(p, TOK_LBRACKET)) {
            // Index
            ASTNode *idx = parse_expr(p);
            consume(p, TOK_RBRACKET, "Expected ']' after index.");
            ASTNode *index = ast_new(NODE_INDEX, left->line, left->column);
            index->index.object = left;
            index->index.index = idx;
            left = index;
        } else if (match(p, TOK_PIPEGT)) {
            // Pipe operator: x |> f becomes f(x)
            ASTNode *fn = parse_postfix(p);
            ASTNode *call = ast_new(NODE_CALL, left->line, left->column);
            call->call.callee = fn;
            call->call.args = malloc(sizeof(ASTNode*));
            call->call.args[0] = left;
            call->call.arg_count = 1;
            left = call;
        } else {
            break;
        }
    }

    return left;
}

static ASTNode *parse_primary(Parser *p) {
    if (match(p, TOK_INT_LIT)) {
        ASTNode *node = ast_new(NODE_LITERAL_INT, p->previous.line, p->previous.column);
        node->int_val = p->previous.int_value;
        return node;
    }

    if (match(p, TOK_FLOAT_LIT)) {
        ASTNode *node = ast_new(NODE_LITERAL_FLOAT, p->previous.line, p->previous.column);
        node->float_val = p->previous.float_value;
        return node;
    }

    if (match(p, TOK_STRING_LIT)) {
        ASTNode *node = ast_new(NODE_LITERAL_STRING, p->previous.line, p->previous.column);
        // Remove quotes and interpret escape sequences
        node->string_val = str_dup_unescape(p->previous.start + 1, p->previous.length - 2);
        return node;
    }

    if (match(p, TOK_TRUE)) {
        ASTNode *node = ast_new(NODE_LITERAL_BOOL, p->previous.line, p->previous.column);
        node->bool_val = true;
        return node;
    }

    if (match(p, TOK_FALSE)) {
        ASTNode *node = ast_new(NODE_LITERAL_BOOL, p->previous.line, p->previous.column);
        node->bool_val = false;
        return node;
    }

    if (match(p, TOK_IDENT)) {
        char *name = str_dup(p->previous.start, p->previous.length);

        // Check for enum variant access: EnumName::Variant
        if (match(p, TOK_COLONCOLON)) {
            consume(p, TOK_IDENT, "Expected variant name after '::'.");
            ASTNode *node = ast_new(NODE_ENUM_VARIANT, p->previous.line, p->previous.column);
            node->enum_variant.enum_name = name;
            node->enum_variant.variant_name = str_dup(p->previous.start, p->previous.length);
            return node;
        }

        // Check for struct initializer: Name { ... }
        if (check(p, TOK_LBRACE)) {
            advance(p);
            ASTNode *node = ast_new(NODE_STRUCT_INIT, p->previous.line, p->previous.column);
            node->struct_init.struct_name = name;
            node->struct_init.field_names = NULL;
            node->struct_init.field_values = NULL;
            node->struct_init.field_count = 0;

            if (!check(p, TOK_RBRACE)) {
                do {
                    consume(p, TOK_IDENT, "Expected field name.");
                    char *field_name = str_dup(p->previous.start, p->previous.length);
                    consume(p, TOK_EQ, "Expected '=' after field name.");
                    ASTNode *field_value = parse_expr(p);

                    node->struct_init.field_count++;
                    node->struct_init.field_names = safe_realloc(node->struct_init.field_names,
                        sizeof(char*) * node->struct_init.field_count);
                    node->struct_init.field_values = safe_realloc(node->struct_init.field_values,
                        sizeof(ASTNode*) * node->struct_init.field_count);
                    node->struct_init.field_names[node->struct_init.field_count - 1] = field_name;
                    node->struct_init.field_values[node->struct_init.field_count - 1] = field_value;
                } while (match(p, TOK_COMMA));
            }

            consume(p, TOK_RBRACE, "Expected '}' after struct initializer.");
            return node;
        }

        ASTNode *node = ast_new(NODE_IDENT, p->previous.line, p->previous.column);
        node->ident = name;
        return node;
    }

    if (match(p, TOK_LPAREN)) {
        ASTNode *expr = parse_expr(p);
        consume(p, TOK_RPAREN, "Expected ')' after expression.");
        return expr;
    }

    if (match(p, TOK_LBRACKET)) {
        // Array initializer: [1, 2, 3]
        ASTNode *node = ast_new(NODE_ARRAY_INIT, p->previous.line, p->previous.column);
        node->array_init.elements = NULL;
        node->array_init.elem_count = 0;

        if (!check(p, TOK_RBRACKET)) {
            do {
                ASTNode *elem = parse_expr(p);
                node->array_init.elem_count++;
                node->array_init.elements = safe_realloc(node->array_init.elements,
                    sizeof(ASTNode*) * node->array_init.elem_count);
                node->array_init.elements[node->array_init.elem_count - 1] = elem;
            } while (match(p, TOK_COMMA));
        }

        consume(p, TOK_RBRACKET, "Expected ']' after array elements.");
        return node;
    }

    error_at(p, &p->current, "Expected expression.");
    return NULL;
}

// Debug printing
static void indent_print(int indent) {
    for (int i = 0; i < indent; i++) printf("  ");
}

void ast_print(ASTNode *node, int indent) {
    if (!node) {
        indent_print(indent);
        printf("(null)\n");
        return;
    }

    indent_print(indent);

    switch (node->kind) {
        case NODE_PROGRAM:
            printf("Program:\n");
            for (int i = 0; i < node->program.decl_count; i++) {
                ast_print(node->program.decls[i], indent + 1);
            }
            break;
        case NODE_FN_DECL:
            printf("FnDecl: %s\n", node->fn_decl.name);
            for (int i = 0; i < node->fn_decl.param_count; i++) {
                ast_print(node->fn_decl.params[i], indent + 1);
            }
            if (node->fn_decl.body) {
                ast_print(node->fn_decl.body, indent + 1);
            }
            break;
        case NODE_VAR_DECL:
            printf("VarDecl: %s%s\n", node->var_decl.is_mut ? "mut " : "", node->var_decl.name);
            if (node->var_decl.init) {
                ast_print(node->var_decl.init, indent + 1);
            }
            break;
        case NODE_PARAM:
            printf("Param: %s\n", node->param.name);
            break;
        case NODE_BLOCK:
            printf("Block:\n");
            for (int i = 0; i < node->block.stmt_count; i++) {
                ast_print(node->block.stmts[i], indent + 1);
            }
            break;
        case NODE_RETURN:
            printf("Return:\n");
            if (node->ret.value) {
                ast_print(node->ret.value, indent + 1);
            }
            break;
        case NODE_BINARY:
            printf("Binary: op=%d\n", node->binary.op);
            ast_print(node->binary.left, indent + 1);
            ast_print(node->binary.right, indent + 1);
            break;
        case NODE_CALL:
            printf("Call:\n");
            ast_print(node->call.callee, indent + 1);
            for (int i = 0; i < node->call.arg_count; i++) {
                ast_print(node->call.args[i], indent + 1);
            }
            break;
        case NODE_LITERAL_INT:
            printf("Int: %lld\n", (long long)node->int_val);
            break;
        case NODE_LITERAL_STRING:
            printf("String: \"%s\"\n", node->string_val);
            break;
        case NODE_IDENT:
            printf("Ident: %s\n", node->ident);
            break;
        case NODE_USE:
            printf("Use: %s\n", node->use.path);
            break;
        case NODE_MEMBER:
            printf("Member: .%s\n", node->member.member);
            ast_print(node->member.object, indent + 1);
            break;
        default:
            printf("Node: kind=%d\n", node->kind);
            break;
    }
}
