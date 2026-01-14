#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Build an index of line starts for error reporting
static void build_line_index(Lexer *lexer) {
    lexer->line_capacity = 64;
    lexer->line_starts = malloc(sizeof(const char*) * lexer->line_capacity);
    lexer->line_count = 0;

    // First line starts at source
    lexer->line_starts[lexer->line_count++] = lexer->source;

    const char *p = lexer->source;
    while (*p) {
        if (*p == '\n') {
            // Ensure capacity
            if (lexer->line_count >= lexer->line_capacity) {
                lexer->line_capacity *= 2;
                lexer->line_starts = realloc(lexer->line_starts,
                    sizeof(const char*) * lexer->line_capacity);
            }
            lexer->line_starts[lexer->line_count++] = p + 1;
        }
        p++;
    }
}

void lexer_init(Lexer *lexer, const char *source) {
    lexer->source = source;
    lexer->start = source;
    lexer->current = source;
    lexer->line = 1;
    lexer->column = 1;
    lexer->start_column = 1;

    // Build line index for error messages
    build_line_index(lexer);
}

static int is_at_end(Lexer *lexer) {
    return *lexer->current == '\0';
}

static char advance(Lexer *lexer) {
    lexer->column++;
    return *lexer->current++;
}

static char peek(Lexer *lexer) {
    return *lexer->current;
}

static char peek_next(Lexer *lexer) {
    if (is_at_end(lexer)) return '\0';
    return lexer->current[1];
}

static int match(Lexer *lexer, char expected) {
    if (is_at_end(lexer)) return 0;
    if (*lexer->current != expected) return 0;
    lexer->current++;
    lexer->column++;
    return 1;
}

static void skip_whitespace(Lexer *lexer) {
    for (;;) {
        char c = peek(lexer);
        switch (c) {
            case ' ':
            case '\t':
            case '\r':
                advance(lexer);
                break;
            case '-':
                // -- comment
                if (peek_next(lexer) == '-') {
                    // Check for multi-line comment ---
                    // Safe bounds check: ensure we have at least 3 characters
                    if (lexer->current[1] != '\0' && lexer->current[2] != '\0' && lexer->current[2] == '-') {
                        advance(lexer); // -
                        advance(lexer); // -
                        advance(lexer); // -
                        // Find closing ---
                        while (!is_at_end(lexer)) {
                            // Safe bounds check before accessing current[2]
                            if (peek(lexer) == '-' && peek_next(lexer) == '-' &&
                                lexer->current[1] != '\0' && lexer->current[2] != '\0' && lexer->current[2] == '-') {
                                advance(lexer);
                                advance(lexer);
                                advance(lexer);
                                break;
                            }
                            if (peek(lexer) == '\n') {
                                lexer->line++;
                                lexer->column = 0;
                            }
                            advance(lexer);
                        }
                    } else {
                        // Single-line comment
                        while (peek(lexer) != '\n' && !is_at_end(lexer)) {
                            advance(lexer);
                        }
                    }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static Token make_token(Lexer *lexer, TokenType type) {
    Token token;
    token.type = type;
    token.start = lexer->start;
    token.length = (size_t)(lexer->current - lexer->start);
    token.line = lexer->line;
    token.column = lexer->start_column;
    token.int_value = 0;
    return token;
}

static Token error_token(Lexer *lexer, const char *message) {
    Token token;
    token.type = TOK_ERROR;
    token.start = message;
    token.length = strlen(message);
    token.line = lexer->line;
    token.column = lexer->start_column;
    token.int_value = 0;
    return token;
}

static int is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static int is_digit(char c) {
    return c >= '0' && c <= '9';
}

static int is_alnum(char c) {
    return is_alpha(c) || is_digit(c);
}

static TokenType check_keyword(Lexer *lexer, int start, int length, const char *rest, TokenType type) {
    if (lexer->current - lexer->start == start + length &&
        memcmp(lexer->start + start, rest, length) == 0) {
        return type;
    }
    return TOK_IDENT;
}

static TokenType identifier_type(Lexer *lexer) {
    switch (lexer->start[0]) {
        case 'a':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case 'n': return check_keyword(lexer, 2, 1, "d", TOK_AND);
                    case 's': return check_keyword(lexer, 2, 0, "", TOK_AS);
                }
            }
            break;
        case 'b':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case 'o': return check_keyword(lexer, 2, 2, "ol", TOK_BOOL);
                    case 'r': return check_keyword(lexer, 2, 3, "eak", TOK_BREAK);
                }
            }
            break;
        case 'c':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case 'o':
                        if (lexer->current - lexer->start > 3 && lexer->start[3] == 't') {
                            return check_keyword(lexer, 2, 6, "ntinue", TOK_CONTINUE);
                        }
                        return check_keyword(lexer, 2, 3, "nst", TOK_CONST);
                }
            }
            break;
        case 'd': return check_keyword(lexer, 1, 1, "o", TOK_DO);
        case 'e':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case 'l':
                        if (lexer->current - lexer->start > 2) {
                            if (lexer->start[2] == 's') return check_keyword(lexer, 3, 1, "e", TOK_ELSE);
                            if (lexer->start[2] == 'i') return check_keyword(lexer, 3, 1, "f", TOK_ELIF);
                        }
                        break;
                    case 'n': return check_keyword(lexer, 2, 1, "d", TOK_END);
                }
            }
            break;
        case 'f':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case 'n': return check_keyword(lexer, 2, 0, "", TOK_FN);
                    case 'a': return check_keyword(lexer, 2, 3, "lse", TOK_FALSE);
                    case 'o': return check_keyword(lexer, 2, 1, "r", TOK_FOR);
                    case '3': return check_keyword(lexer, 2, 1, "2", TOK_F32);
                    case '6': return check_keyword(lexer, 2, 1, "4", TOK_F64);
                }
            }
            break;
        case 'i':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case 'f': return check_keyword(lexer, 2, 0, "", TOK_IF);
                    case 'n': return check_keyword(lexer, 2, 0, "", TOK_IN);
                    case '8': return check_keyword(lexer, 2, 0, "", TOK_I8);
                    case '1': return check_keyword(lexer, 2, 1, "6", TOK_I16);
                    case '3': return check_keyword(lexer, 2, 1, "2", TOK_I32);
                    case '6': return check_keyword(lexer, 2, 1, "4", TOK_I64);
                }
            }
            break;
        case 'l': return check_keyword(lexer, 1, 2, "et", TOK_LET);
        case 'm':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case 'u': return check_keyword(lexer, 2, 1, "t", TOK_MUT);
                    case 'a': return check_keyword(lexer, 2, 3, "tch", TOK_MATCH);
                }
            }
            break;
        case 'n': return check_keyword(lexer, 1, 2, "ot", TOK_NOT);
        case 'o': return check_keyword(lexer, 1, 1, "r", TOK_OR);
        case 'p': return check_keyword(lexer, 1, 2, "tr", TOK_PTR);
        case 'r': return check_keyword(lexer, 1, 2, "et", TOK_RET);
        case 's': return check_keyword(lexer, 1, 5, "truct", TOK_STRUCT);
        case 't': return check_keyword(lexer, 1, 3, "rue", TOK_TRUE);
        case 'u':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case '8': return check_keyword(lexer, 2, 0, "", TOK_U8);
                    case '1': return check_keyword(lexer, 2, 1, "6", TOK_U16);
                    case '3': return check_keyword(lexer, 2, 1, "2", TOK_U32);
                    case '6': return check_keyword(lexer, 2, 1, "4", TOK_U64);
                }
            }
            break;
        case 'v': return check_keyword(lexer, 1, 3, "oid", TOK_VOID);
        case 'w': return check_keyword(lexer, 1, 4, "hile", TOK_WHILE);
    }
    return TOK_IDENT;
}

static Token identifier(Lexer *lexer) {
    while (is_alnum(peek(lexer))) advance(lexer);
    return make_token(lexer, identifier_type(lexer));
}

static Token number(Lexer *lexer) {
    while (is_digit(peek(lexer))) advance(lexer);

    // Check for float
    if (peek(lexer) == '.' && is_digit(peek_next(lexer))) {
        advance(lexer); // consume '.'
        while (is_digit(peek(lexer))) advance(lexer);

        Token tok = make_token(lexer, TOK_FLOAT_LIT);
        tok.float_value = strtod(lexer->start, NULL);
        return tok;
    }

    Token tok = make_token(lexer, TOK_INT_LIT);
    tok.int_value = strtoll(lexer->start, NULL, 10);
    return tok;
}

static Token string(Lexer *lexer) {
    while (peek(lexer) != '"' && !is_at_end(lexer)) {
        if (peek(lexer) == '\n') {
            lexer->line++;
            lexer->column = 0;
        }
        if (peek(lexer) == '\\' && peek_next(lexer) != '\0') {
            advance(lexer); // skip backslash
        }
        advance(lexer);
    }

    if (is_at_end(lexer)) {
        return error_token(lexer, "Unterminated string.");
    }

    advance(lexer); // closing quote
    return make_token(lexer, TOK_STRING_LIT);
}

static Token directive(Lexer *lexer) {
    // We've already consumed '@', now read the directive name
    while (is_alpha(peek(lexer))) advance(lexer);

    size_t len = lexer->current - lexer->start;
    if (len == 4 && memcmp(lexer->start, "@use", 4) == 0) {
        return make_token(lexer, TOK_DIR_USE);
    } else if (len == 7 && memcmp(lexer->start, "@extern", 7) == 0) {
        return make_token(lexer, TOK_DIR_EXTERN);
    } else if (len == 6 && memcmp(lexer->start, "@alloc", 6) == 0) {
        return make_token(lexer, TOK_DIR_ALLOC);
    } else if (len == 5 && memcmp(lexer->start, "@free", 5) == 0) {
        return make_token(lexer, TOK_DIR_FREE);
    }

    return error_token(lexer, "Unknown directive.");
}

Token lexer_next(Lexer *lexer) {
    skip_whitespace(lexer);

    lexer->start = lexer->current;
    lexer->start_column = lexer->column;

    if (is_at_end(lexer)) return make_token(lexer, TOK_EOF);

    char c = advance(lexer);

    if (is_alpha(c)) return identifier(lexer);
    if (is_digit(c)) return number(lexer);

    switch (c) {
        case '\n':
            lexer->line++;
            lexer->column = 1;
            return make_token(lexer, TOK_NEWLINE);
        case '(': return make_token(lexer, TOK_LPAREN);
        case ')': return make_token(lexer, TOK_RPAREN);
        case '{': return make_token(lexer, TOK_LBRACE);
        case '}': return make_token(lexer, TOK_RBRACE);
        case '[': return make_token(lexer, TOK_LBRACKET);
        case ']': return make_token(lexer, TOK_RBRACKET);
        case ',': return make_token(lexer, TOK_COMMA);
        case ';': return make_token(lexer, TOK_SEMICOLON);
        case '~': return make_token(lexer, TOK_TILDE);
        case '?': return make_token(lexer, TOK_QUESTION);
        case '@': return directive(lexer);
        case '+': return make_token(lexer, TOK_PLUS);
        case '*': return make_token(lexer, TOK_STAR);
        case '/': return make_token(lexer, TOK_SLASH);
        case '%': return make_token(lexer, TOK_PERCENT);
        case '^': return make_token(lexer, TOK_CARET);
        case '"': return string(lexer);

        case '.':
            if (match(lexer, '.')) return make_token(lexer, TOK_DOTDOT);
            return make_token(lexer, TOK_DOT);

        case ':':
            if (match(lexer, ':')) return make_token(lexer, TOK_COLONCOLON);
            return make_token(lexer, TOK_COLON);

        case '-':
            if (match(lexer, '>')) return make_token(lexer, TOK_ARROW);
            return make_token(lexer, TOK_MINUS);

        case '=':
            if (match(lexer, '=')) return make_token(lexer, TOK_EQEQ);
            if (match(lexer, '>')) return make_token(lexer, TOK_FATARROW);
            return make_token(lexer, TOK_EQ);

        case '!':
            if (match(lexer, '=')) return make_token(lexer, TOK_NE);
            return error_token(lexer, "Expected '=' after '!'.");

        case '<':
            if (match(lexer, '=')) return make_token(lexer, TOK_LE);
            if (match(lexer, '<')) return make_token(lexer, TOK_LSHIFT);
            return make_token(lexer, TOK_LT);

        case '>':
            if (match(lexer, '=')) return make_token(lexer, TOK_GE);
            if (match(lexer, '>')) return make_token(lexer, TOK_RSHIFT);
            return make_token(lexer, TOK_GT);

        case '&':
            return make_token(lexer, TOK_AMP);

        case '|':
            if (match(lexer, '>')) return make_token(lexer, TOK_PIPEGT);
            return make_token(lexer, TOK_PIPE);
    }

    return error_token(lexer, "Unexpected character.");
}

Token lexer_peek(Lexer *lexer) {
    const char *saved_start = lexer->start;
    const char *saved_current = lexer->current;
    int saved_line = lexer->line;
    int saved_column = lexer->column;
    int saved_start_column = lexer->start_column;

    Token tok = lexer_next(lexer);

    lexer->start = saved_start;
    lexer->current = saved_current;
    lexer->line = saved_line;
    lexer->column = saved_column;
    lexer->start_column = saved_start_column;

    return tok;
}

const char *token_type_name(TokenType type) {
    switch (type) {
        case TOK_INT_LIT: return "INT_LIT";
        case TOK_FLOAT_LIT: return "FLOAT_LIT";
        case TOK_STRING_LIT: return "STRING_LIT";
        case TOK_IDENT: return "IDENT";
        case TOK_FN: return "fn";
        case TOK_LET: return "let";
        case TOK_MUT: return "mut";
        case TOK_CONST: return "const";
        case TOK_STRUCT: return "struct";
        case TOK_IF: return "if";
        case TOK_ELIF: return "elif";
        case TOK_ELSE: return "else";
        case TOK_WHILE: return "while";
        case TOK_FOR: return "for";
        case TOK_IN: return "in";
        case TOK_MATCH: return "match";
        case TOK_RET: return "ret";
        case TOK_BREAK: return "break";
        case TOK_CONTINUE: return "continue";
        case TOK_DO: return "do";
        case TOK_END: return "end";
        case TOK_AND: return "and";
        case TOK_OR: return "or";
        case TOK_NOT: return "not";
        case TOK_TRUE: return "true";
        case TOK_FALSE: return "false";
        case TOK_AS: return "as";
        case TOK_I8: return "i8";
        case TOK_I16: return "i16";
        case TOK_I32: return "i32";
        case TOK_I64: return "i64";
        case TOK_U8: return "u8";
        case TOK_U16: return "u16";
        case TOK_U32: return "u32";
        case TOK_U64: return "u64";
        case TOK_F32: return "f32";
        case TOK_F64: return "f64";
        case TOK_BOOL: return "bool";
        case TOK_VOID: return "void";
        case TOK_PTR: return "ptr";
        case TOK_PLUS: return "+";
        case TOK_MINUS: return "-";
        case TOK_STAR: return "*";
        case TOK_SLASH: return "/";
        case TOK_PERCENT: return "%";
        case TOK_AMP: return "&";
        case TOK_PIPE: return "|";
        case TOK_CARET: return "^";
        case TOK_TILDE: return "~";
        case TOK_LSHIFT: return "<<";
        case TOK_RSHIFT: return ">>";
        case TOK_EQ: return "=";
        case TOK_EQEQ: return "==";
        case TOK_NE: return "!=";
        case TOK_LT: return "<";
        case TOK_GT: return ">";
        case TOK_LE: return "<=";
        case TOK_GE: return ">=";
        case TOK_ARROW: return "->";
        case TOK_FATARROW: return "=>";
        case TOK_COLONCOLON: return "::";
        case TOK_DOTDOT: return "..";
        case TOK_PIPEGT: return "|>";
        case TOK_QUESTION: return "?";
        case TOK_LPAREN: return "(";
        case TOK_RPAREN: return ")";
        case TOK_LBRACKET: return "[";
        case TOK_RBRACKET: return "]";
        case TOK_LBRACE: return "{";
        case TOK_RBRACE: return "}";
        case TOK_COMMA: return ",";
        case TOK_DOT: return ".";
        case TOK_COLON: return ":";
        case TOK_SEMICOLON: return ";";
        case TOK_AT: return "@";
        case TOK_DIR_USE: return "@use";
        case TOK_DIR_EXTERN: return "@extern";
        case TOK_DIR_ALLOC: return "@alloc";
        case TOK_DIR_FREE: return "@free";
        case TOK_NEWLINE: return "NEWLINE";
        case TOK_EOF: return "EOF";
        case TOK_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void token_print(Token *tok) {
    printf("[%d:%d] %s", tok->line, tok->column, token_type_name(tok->type));
    if (tok->type == TOK_IDENT || tok->type == TOK_STRING_LIT) {
        printf(" '%.*s'", (int)tok->length, tok->start);
    } else if (tok->type == TOK_INT_LIT) {
        printf(" %lld", (long long)tok->int_value);
    } else if (tok->type == TOK_FLOAT_LIT) {
        printf(" %f", tok->float_value);
    } else if (tok->type == TOK_ERROR) {
        printf(" '%s'", tok->start);
    }
    printf("\n");
}

const char *lexer_get_line(Lexer *lexer, int line_num) {
    if (line_num < 1 || line_num > lexer->line_count) {
        return NULL;
    }
    return lexer->line_starts[line_num - 1];  // Convert to 0-indexed
}

int lexer_get_line_length(Lexer *lexer, int line_num) {
    const char *line = lexer_get_line(lexer, line_num);
    if (!line) return 0;

    int len = 0;
    while (line[len] && line[len] != '\n') {
        len++;
    }
    return len;
}

void lexer_free(Lexer *lexer) {
    if (lexer->line_starts) {
        free(lexer->line_starts);
        lexer->line_starts = NULL;
    }
    lexer->line_count = 0;
    lexer->line_capacity = 0;
}
