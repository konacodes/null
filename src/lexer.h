#ifndef NULL_LEXER_H
#define NULL_LEXER_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    // Literals
    TOK_INT_LIT,        // 42
    TOK_FLOAT_LIT,      // 3.14
    TOK_STRING_LIT,     // "hello"
    TOK_IDENT,          // variable/function names

    // Keywords
    TOK_FN,             // fn
    TOK_LET,            // let
    TOK_MUT,            // mut
    TOK_CONST,          // const
    TOK_STRUCT,         // struct
    TOK_ENUM,           // enum
    TOK_IF,             // if
    TOK_ELIF,           // elif
    TOK_ELSE,           // else
    TOK_WHILE,          // while
    TOK_FOR,            // for
    TOK_IN,             // in
    TOK_MATCH,          // match
    TOK_RET,            // ret
    TOK_BREAK,          // break
    TOK_CONTINUE,       // continue
    TOK_DO,             // do
    TOK_END,            // end
    TOK_AND,            // and
    TOK_OR,             // or
    TOK_NOT,            // not
    TOK_TRUE,           // true
    TOK_FALSE,          // false
    TOK_AS,             // as

    // Types
    TOK_I8,
    TOK_I16,
    TOK_I32,
    TOK_I64,
    TOK_U8,
    TOK_U16,
    TOK_U32,
    TOK_U64,
    TOK_F32,
    TOK_F64,
    TOK_BOOL,
    TOK_VOID,
    TOK_PTR,

    // Operators
    TOK_PLUS,           // +
    TOK_MINUS,          // -
    TOK_STAR,           // *
    TOK_SLASH,          // /
    TOK_PERCENT,        // %
    TOK_AMP,            // &
    TOK_PIPE,           // |
    TOK_CARET,          // ^
    TOK_TILDE,          // ~
    TOK_LSHIFT,         // <<
    TOK_RSHIFT,         // >>
    TOK_EQ,             // =
    TOK_EQEQ,           // ==
    TOK_NE,             // !=
    TOK_LT,             // <
    TOK_GT,             // >
    TOK_LE,             // <=
    TOK_GE,             // >=
    TOK_ARROW,          // ->
    TOK_FATARROW,       // =>
    TOK_COLONCOLON,     // ::
    TOK_DOTDOT,         // ..
    TOK_PIPEGT,         // |>
    TOK_QUESTION,       // ?

    // Delimiters
    TOK_LPAREN,         // (
    TOK_RPAREN,         // )
    TOK_LBRACKET,       // [
    TOK_RBRACKET,       // ]
    TOK_LBRACE,         // {
    TOK_RBRACE,         // }
    TOK_COMMA,          // ,
    TOK_DOT,            // .
    TOK_COLON,          // :
    TOK_SEMICOLON,      // ;
    TOK_AT,             // @

    // Directives (after @)
    TOK_DIR_USE,        // use (in @use)
    TOK_DIR_EXTERN,     // extern (in @extern)
    TOK_DIR_ALLOC,      // alloc (in @alloc)
    TOK_DIR_FREE,       // free (in @free)

    // Special
    TOK_NEWLINE,        // significant newlines
    TOK_EOF,            // end of file
    TOK_ERROR           // lexer error
} TokenType;

typedef struct {
    TokenType type;
    const char *start;
    size_t length;
    int line;
    int column;

    // For literals
    union {
        int64_t int_value;
        double float_value;
    };
} Token;

typedef struct {
    const char *source;
    const char *start;
    const char *current;
    int line;
    int column;
    int start_column;

    // Source line tracking for error messages
    const char **line_starts;  // Array of pointers to line starts
    int line_count;            // Number of lines
    int line_capacity;         // Capacity of line_starts array
} Lexer;

// Initialize lexer with source code
void lexer_init(Lexer *lexer, const char *source);

// Get next token
Token lexer_next(Lexer *lexer);

// Peek at next token without consuming
Token lexer_peek(Lexer *lexer);

// Get string representation of token type
const char *token_type_name(TokenType type);

// Print token for debugging
void token_print(Token *tok);

// Get pointer to the start of a line (1-indexed)
const char *lexer_get_line(Lexer *lexer, int line_num);

// Get the length of a line (excluding newline)
int lexer_get_line_length(Lexer *lexer, int line_num);

// Free lexer resources
void lexer_free(Lexer *lexer);

#endif // NULL_LEXER_H
