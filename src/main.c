#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "lexer.h"
#include "parser.h"
#include "analyzer.h"
#include "codegen.h"

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Could not open file: %s\n", path);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buffer = malloc(size + 1);
    if (!buffer) {
        fclose(f);
        return NULL;
    }

    size_t read = fread(buffer, 1, size, f);
    buffer[read] = '\0';
    fclose(f);
    return buffer;
}

static char *get_std_path(void) {
    // Try relative to executable first
    static char path[1024];

    // Try current directory
    struct stat st;
    if (stat("std", &st) == 0 && S_ISDIR(st.st_mode)) {
        return "std";
    }

    // Try relative to executable
    // This is a simplified approach - in production you'd use platform-specific
    // methods to find the executable path
    return "/home/jace/projects/null/std";
}

// Simple preprocessor for @use directives
static char *preprocess(const char *source, const char *base_path) {
    // For now, just strip @use directives and add extern declarations
    // A real implementation would parse imports and include the relevant code

    // Find all @use directives and build a combined source
    size_t capacity = strlen(source) * 2 + 4096;
    char *result = malloc(capacity);
    result[0] = '\0';

    // Add standard library extern declarations
    strcat(result, "@extern \"C\" do\n");
    strcat(result, "    fn puts(s :: ptr<u8>) -> i32\n");
    strcat(result, "    fn printf(fmt :: ptr<u8>) -> i32\n");
    strcat(result, "    fn exit(code :: i32) -> void\n");
    strcat(result, "end\n\n");

    // Add io module functions as wrappers
    strcat(result, "fn io_print(s :: ptr<u8>) -> void do\n");
    strcat(result, "    puts(s)\n");
    strcat(result, "end\n\n");

    // Copy the rest, skipping @use lines
    const char *p = source;
    while (*p) {
        // Skip @use lines
        if (strncmp(p, "@use", 4) == 0) {
            while (*p && *p != '\n') p++;
            if (*p == '\n') p++;
            continue;
        }

        // Copy character
        size_t len = strlen(result);
        result[len] = *p;
        result[len + 1] = '\0';
        p++;
    }

    return result;
}

static void print_usage(const char *prog) {
    printf("null - A compiled programming language\n\n");
    printf("Usage:\n");
    printf("  %s <file.null>           Run the program\n", prog);
    printf("  %s run <file.null>       Run the program\n", prog);
    printf("  %s build <file.null> -o <output>   Compile to executable\n", prog);
    printf("  %s test <dir>            Run tests in directory\n", prog);
    printf("  %s --help                Show this help\n", prog);
}

static int compile_and_run(const char *filename) {
    char *source = read_file(filename);
    if (!source) return 1;

    // Preprocess (handle imports)
    char *processed = preprocess(source, get_std_path());
    free(source);

    // Lex
    Lexer lexer;
    lexer_init(&lexer, processed);

    // Parse
    Parser parser;
    parser_init(&parser, &lexer);
    ASTNode *ast = parser_parse(&parser);

    if (parser.had_error) {
        ast_free(ast);
        free(processed);
        return 1;
    }

    // Analyze
    Analyzer analyzer;
    analyzer_init(&analyzer);
    if (!analyzer_analyze(&analyzer, ast)) {
        ast_free(ast);
        analyzer_free(&analyzer);
        free(processed);
        return 1;
    }

    // Codegen
    Codegen cg;
    codegen_init(&cg, filename);

    if (!codegen_generate(&cg, ast)) {
        ast_free(ast);
        analyzer_free(&analyzer);
        codegen_free(&cg);
        free(processed);
        return 1;
    }

    // Run
    int result = codegen_jit_run(&cg);

    analyzer_free(&analyzer);
    ast_free(ast);
    codegen_free(&cg);
    free(processed);

    return result;
}

static int compile_to_executable(const char *filename, const char *output) {
    char *source = read_file(filename);
    if (!source) return 1;

    // Preprocess
    char *processed = preprocess(source, get_std_path());
    free(source);

    // Lex
    Lexer lexer;
    lexer_init(&lexer, processed);

    // Parse
    Parser parser;
    parser_init(&parser, &lexer);
    ASTNode *ast = parser_parse(&parser);

    if (parser.had_error) {
        ast_free(ast);
        free(processed);
        return 1;
    }

    // Analyze
    Analyzer analyzer;
    analyzer_init(&analyzer);
    if (!analyzer_analyze(&analyzer, ast)) {
        ast_free(ast);
        analyzer_free(&analyzer);
        free(processed);
        return 1;
    }

    // Codegen
    Codegen cg;
    codegen_init(&cg, filename);

    if (!codegen_generate(&cg, ast)) {
        ast_free(ast);
        analyzer_free(&analyzer);
        codegen_free(&cg);
        free(processed);
        return 1;
    }

    // Emit object file
    char obj_file[256];
    snprintf(obj_file, sizeof(obj_file), "/tmp/null_%d.o", getpid());

    if (!codegen_emit_object(&cg, obj_file)) {
        ast_free(ast);
        analyzer_free(&analyzer);
        codegen_free(&cg);
        free(processed);
        return 1;
    }

    // Link with clang
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "clang %s -o %s -lm", obj_file, output);
    int result = system(cmd);

    // Clean up
    remove(obj_file);
    analyzer_free(&analyzer);
    ast_free(ast);
    codegen_free(&cg);
    free(processed);

    return result == 0 ? 0 : 1;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_usage(argv[0]);
        return 0;
    }

    if (strcmp(argv[1], "run") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s run <file.null>\n", argv[0]);
            return 1;
        }
        return compile_and_run(argv[2]);
    }

    if (strcmp(argv[1], "build") == 0) {
        if (argc < 5 || strcmp(argv[3], "-o") != 0) {
            fprintf(stderr, "Usage: %s build <file.null> -o <output>\n", argv[0]);
            return 1;
        }
        return compile_to_executable(argv[2], argv[4]);
    }

    if (strcmp(argv[1], "test") == 0) {
        // Run tests in directory
        // For now, just compile and run each .null file
        printf("Running tests...\n");
        // TODO: implement test runner
        return 0;
    }

    // Default: run the file
    return compile_and_run(argv[1]);
}
