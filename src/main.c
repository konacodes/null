#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#include <dirent.h>
#include "lexer.h"
#include "parser.h"
#include "analyzer.h"
#include "codegen.h"

#define MAX_SOURCE_SIZE (10 * 1024 * 1024)  // 10 MB max source file size

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Could not open file: %s\n", path);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size < 0) {
        fprintf(stderr, "Could not determine file size: %s\n", path);
        fclose(f);
        return NULL;
    }

    if (size > MAX_SOURCE_SIZE) {
        fprintf(stderr, "Source file too large (max %d bytes): %s\n", MAX_SOURCE_SIZE, path);
        fclose(f);
        return NULL;
    }

    char *buffer = malloc(size + 1);
    if (!buffer) {
        fprintf(stderr, "Out of memory allocating %ld bytes\n", size);
        fclose(f);
        return NULL;
    }

    size_t bytes_read = fread(buffer, 1, size, f);
    buffer[bytes_read] = '\0';
    fclose(f);
    return buffer;
}

static char *get_std_path(void) {
    static char path[PATH_MAX];
    struct stat st;

    // Try current directory first
    if (stat("std", &st) == 0 && S_ISDIR(st.st_mode)) {
        return "std";
    }

    // Try relative to executable using /proc/self/exe (Linux)
    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len != -1) {
        exe_path[len] = '\0';
        // dirname may modify the string, so work with a copy
        char *dir = dirname(exe_path);
        snprintf(path, sizeof(path), "%s/std", dir);
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            return path;
        }
        // Try parent directory (for when running from build/)
        snprintf(path, sizeof(path), "%s/../std", dir);
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            return path;
        }
    }

    // Fallback to current directory
    return "./std";
}

// Simple preprocessor for @use directives
static char *preprocess(const char *source, const char *base_path) {
    (void)base_path;  // Unused for now
    // For now, just strip @use directives and add extern declarations
    // A real implementation would parse imports and include the relevant code

    // Find all @use directives and build a combined source
    size_t source_len = strlen(source);
    size_t capacity = source_len * 2 + 4096;
    char *result = malloc(capacity);
    if (!result) {
        fprintf(stderr, "Out of memory in preprocessor\n");
        return NULL;
    }

    // Build header with standard library extern declarations
    const char *header =
        "@extern \"C\" do\n"
        "    fn puts(s :: ptr<u8>) -> i32\n"
        "    fn printf(fmt :: ptr<u8>) -> i32\n"
        "    fn exit(code :: i32) -> void\n"
        "end\n\n"
        "fn io_print(s :: ptr<u8>) -> void do\n"
        "    puts(s)\n"
        "end\n\n";

    size_t header_len = strlen(header);
    memcpy(result, header, header_len);
    size_t result_len = header_len;

    // Copy the rest, skipping @use lines - track length explicitly for O(n)
    const char *p = source;
    while (*p) {
        // Skip @use lines
        if (strncmp(p, "@use", 4) == 0) {
            while (*p && *p != '\n') p++;
            if (*p == '\n') p++;
            continue;
        }

        // Copy character - O(1) operation now
        result[result_len++] = *p++;
    }
    result[result_len] = '\0';

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

    // Link with clang - use fork/exec to avoid command injection
    int result = 1;
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        execlp("clang", "clang", obj_file, "-o", output, "-lm", (char*)NULL);
        // If execlp returns, it failed
        _exit(127);
    } else if (pid > 0) {
        // Parent process - wait for child
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            result = WEXITSTATUS(status);
        }
    } else {
        fprintf(stderr, "Failed to fork for linking\n");
    }

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
        // Run tests in a directory
        const char *test_dir = (argc >= 3) ? argv[2] : "tests";
        DIR *dir = opendir(test_dir);
        if (!dir) {
            fprintf(stderr, "Could not open test directory: %s\n", test_dir);
            return 1;
        }

        int passed = 0, failed = 0;
        struct dirent *entry;
        printf("Running tests in %s...\n", test_dir);

        while ((entry = readdir(dir)) != NULL) {
            size_t name_len = strlen(entry->d_name);
            if (name_len > 5 && strcmp(entry->d_name + name_len - 5, ".null") == 0) {
                char path[PATH_MAX];
                snprintf(path, sizeof(path), "%s/%s", test_dir, entry->d_name);

                printf("  Testing %s... ", entry->d_name);
                fflush(stdout);

                int result = compile_and_run(path);
                if (result == 0) {
                    printf("OK\n");
                    passed++;
                } else {
                    printf("FAIL (exit %d)\n", result);
                    failed++;
                }
            }
        }
        closedir(dir);

        printf("\nResults: %d passed, %d failed\n", passed, failed);
        return failed > 0 ? 1 : 0;
    }

    // Default: run the file
    return compile_and_run(argv[1]);
}
