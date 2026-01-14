#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#include <dirent.h>
#include <sys/resource.h>
#include <signal.h>
#include "lexer.h"
#include "parser.h"
#include "analyzer.h"
#include "codegen.h"
#include "interp.h"

#define MAX_SOURCE_SIZE (10 * 1024 * 1024)  // 10 MB max source file size
#define MAX_MEMORY_MB 4096  // 4 GB max memory usage
#define MAX_CPU_SECONDS 120  // 2 minute max compilation time

// Set resource limits to prevent runaway memory/CPU usage
static void set_resource_limits(void) {
    struct rlimit limit;

    // Limit virtual memory to prevent OOM
    limit.rlim_cur = (rlim_t)MAX_MEMORY_MB * 1024 * 1024;
    limit.rlim_max = (rlim_t)MAX_MEMORY_MB * 1024 * 1024;
    if (setrlimit(RLIMIT_AS, &limit) != 0) {
        // Non-fatal, just warn
        // fprintf(stderr, "Warning: Could not set memory limit\n");
    }

    // Limit CPU time to prevent infinite loops
    limit.rlim_cur = MAX_CPU_SECONDS;
    limit.rlim_max = MAX_CPU_SECONDS + 10;
    if (setrlimit(RLIMIT_CPU, &limit) != 0) {
        // Non-fatal
    }
}

// Signal handler for resource limit exceeded
static void resource_limit_handler(int sig) {
    if (sig == SIGXCPU) {
        fprintf(stderr, "\nError: Compilation exceeded CPU time limit (%d seconds)\n", MAX_CPU_SECONDS);
    } else if (sig == SIGSEGV) {
        fprintf(stderr, "\nError: Segmentation fault - possible memory corruption or stack overflow\n");
    }
    _exit(1);
}

static void setup_signal_handlers(void) {
    signal(SIGXCPU, resource_limit_handler);
    // Don't catch SIGSEGV - let it crash normally for debugging
}

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

// Module system for @use directives
#define MAX_MODULES 64

typedef struct {
    char *paths[MAX_MODULES];
    int count;
} ImportedModules;

static ImportedModules imported_modules = {0};

static void reset_imported_modules(void) {
    for (int i = 0; i < imported_modules.count; i++) {
        free(imported_modules.paths[i]);
    }
    imported_modules.count = 0;
}

static bool is_module_imported(const char *path) {
    for (int i = 0; i < imported_modules.count; i++) {
        if (strcmp(imported_modules.paths[i], path) == 0) {
            return true;
        }
    }
    return false;
}

static bool mark_module_imported(const char *path) {
    if (imported_modules.count >= MAX_MODULES) {
        fprintf(stderr, "Too many modules imported (max %d)\n", MAX_MODULES);
        return false;
    }
    imported_modules.paths[imported_modules.count++] = strdup(path);
    return true;
}

static char *resolve_module_path(const char *module_path, const char *base_path) {
    static char resolved[PATH_MAX];

    // If path starts with "std/", use standard library path
    if (strncmp(module_path, "std/", 4) == 0) {
        snprintf(resolved, sizeof(resolved), "%s/%s", get_std_path(), module_path + 4);
    }
    // If path starts with "./", resolve relative to base
    else if (strncmp(module_path, "./", 2) == 0) {
        char base_copy[PATH_MAX];
        strncpy(base_copy, base_path, sizeof(base_copy) - 1);
        base_copy[sizeof(base_copy) - 1] = '\0';
        char *dir = dirname(base_copy);
        snprintf(resolved, sizeof(resolved), "%s/%s", dir, module_path + 2);
    }
    // Otherwise treat as relative to current directory
    else {
        snprintf(resolved, sizeof(resolved), "%s", module_path);
    }

    return resolved;
}

// Maximum preprocessed output size (50 MB) - prevents runaway memory growth
#define MAX_PREPROCESSED_SIZE (50 * 1024 * 1024)

// Internal preprocess function - is_toplevel controls whether to add builtin header
static char *preprocess_internal(const char *source, const char *base_path, bool is_toplevel) {
    size_t source_len = strlen(source);
    size_t capacity = source_len * 4 + 8192;  // Extra space for included modules

    // Sanity check - cap initial allocation
    if (capacity > MAX_PREPROCESSED_SIZE) {
        capacity = MAX_PREPROCESSED_SIZE;
    }

    char *result = malloc(capacity);
    if (!result) {
        fprintf(stderr, "Out of memory in preprocessor\n");
        return NULL;
    }

    size_t result_len = 0;

    // Only add builtin header to top-level source that doesn't have @use
    if (is_toplevel) {
        bool has_use_directive = (strstr(source, "@use") != NULL);
        if (!has_use_directive) {
            // Add minimal built-in io_print for backwards compatibility
            const char *builtin_header =
                "@extern \"C\" do\n"
                "    fn puts(s :: ptr<u8>) -> i32\n"
                "end\n"
                "fn io_print(s :: ptr<u8>) -> void do\n"
                "    puts(s)\n"
                "end\n\n";

            size_t header_len = strlen(builtin_header);
            memcpy(result, builtin_header, header_len);
            result_len = header_len;
        }
    }

    const char *p = source;
    while (*p) {
        // Check for @use directive
        if (strncmp(p, "@use", 4) == 0) {
            p += 4;
            // Skip whitespace
            while (*p == ' ' || *p == '\t') p++;

            // Parse the module path (expect "path")
            if (*p == '"') {
                p++;
                const char *path_start = p;
                while (*p && *p != '"' && *p != '\n') p++;

                if (*p == '"') {
                    size_t path_len = p - path_start;
                    char module_path[PATH_MAX];
                    if (path_len < sizeof(module_path)) {
                        strncpy(module_path, path_start, path_len);
                        module_path[path_len] = '\0';

                        // Resolve the full path
                        char *resolved = resolve_module_path(module_path, base_path);

                        // Check for cycles
                        if (!is_module_imported(resolved)) {
                            mark_module_imported(resolved);

                            // Read and preprocess the module
                            char *module_source = read_file(resolved);
                            if (module_source) {
                                char *processed_module = preprocess_internal(module_source, resolved, false);
                                free(module_source);

                                if (processed_module) {
                                    size_t module_len = strlen(processed_module);
                                    // Ensure capacity
                                    while (result_len + module_len + 2 >= capacity) {
                                        capacity *= 2;
                                        if (capacity > MAX_PREPROCESSED_SIZE) {
                                            fprintf(stderr, "Error: Preprocessed output exceeds maximum size (%d MB)\n", MAX_PREPROCESSED_SIZE / (1024 * 1024));
                                            free(result);
                                            free(processed_module);
                                            return NULL;
                                        }
                                        char *new_result = realloc(result, capacity);
                                        if (!new_result) {
                                            free(result);
                                            free(processed_module);
                                            return NULL;
                                        }
                                        result = new_result;
                                    }
                                    memcpy(result + result_len, processed_module, module_len);
                                    result_len += module_len;
                                    result[result_len++] = '\n';
                                    result[result_len] = '\0';
                                    free(processed_module);
                                }
                            }
                        }
                    }
                    p++;  // Skip closing quote
                }
            }
            // Skip to end of line but preserve the newline in output
            while (*p && *p != '\n') p++;
            if (*p == '\n') {
                // Preserve newline to maintain line structure
                result[result_len++] = '\n';
                p++;
            }
            continue;
        }

        // Ensure capacity for next character
        if (result_len + 2 >= capacity) {
            capacity *= 2;
            if (capacity > MAX_PREPROCESSED_SIZE) {
                fprintf(stderr, "Error: Preprocessed output exceeds maximum size\n");
                free(result);
                return NULL;
            }
            char *new_result = realloc(result, capacity);
            if (!new_result) {
                free(result);
                return NULL;
            }
            result = new_result;
        }

        result[result_len++] = *p++;
    }
    result[result_len] = '\0';

    return result;
}

// Public preprocess function - always treats as top-level
static char *preprocess(const char *source, const char *base_path) {
    return preprocess_internal(source, base_path, true);
}

static char *preprocess_file(const char *filepath) {
    char *source = read_file(filepath);
    if (!source) return NULL;

    char *result = preprocess(source, filepath);
    free(source);
    return result;
}

// REPL - Read-Eval-Print Loop
static int run_repl(void) {
    printf("null REPL v0.1\n");
    printf("Type expressions to evaluate, :help for commands, :exit to quit\n\n");

    // Build up a persistent environment (variables, functions, structs)
    char *env_code = malloc(65536);  // Accumulated definitions
    if (!env_code) {
        fprintf(stderr, "Out of memory\n");
        return 1;
    }
    size_t env_len = 0;
    env_code[0] = '\0';

    // Add basic header for io
    const char *header =
        "@extern \"C\" do\n"
        "    fn puts(s :: ptr<u8>) -> i32\n"
        "    fn putchar(c :: i64) -> i64\n"
        "end\n"
        "fn io_print(s :: ptr<u8>) -> void do puts(s) end\n\n";
    strcpy(env_code, header);
    env_len = strlen(header);

    char line[4096];
    int line_num = 1;

    while (1) {
        printf("null:%d> ", line_num);
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[--len] = '\0';
        }

        // Skip empty lines
        if (len == 0) continue;

        // Handle REPL commands
        if (line[0] == ':') {
            if (strcmp(line, ":exit") == 0 || strcmp(line, ":quit") == 0 || strcmp(line, ":q") == 0) {
                printf("Goodbye!\n");
                break;
            }
            if (strcmp(line, ":help") == 0 || strcmp(line, ":h") == 0) {
                printf("REPL Commands:\n");
                printf("  :help, :h     Show this help\n");
                printf("  :exit, :q     Exit the REPL\n");
                printf("  :clear        Clear the environment\n");
                printf("  :env          Show defined names\n");
                printf("  :type <expr>  Show type of expression\n\n");
                printf("Language hints:\n");
                printf("  let x = 42                  Define immutable variable\n");
                printf("  mut y = 10                  Define mutable variable\n");
                printf("  fn add(a :: i64, b :: i64) -> i64 do ret a + b end\n");
                printf("  io_print(\"hello\")           Print a string\n");
                continue;
            }
            if (strcmp(line, ":clear") == 0) {
                env_len = strlen(header);
                env_code[env_len] = '\0';
                printf("Environment cleared.\n");
                continue;
            }
            if (strcmp(line, ":env") == 0) {
                printf("Current environment code:\n%s\n", env_code + strlen(header));
                continue;
            }
            printf("Unknown command: %s (type :help for commands)\n", line);
            continue;
        }

        // Check if it's a definition (fn, let, mut, struct) or an expression
        bool is_definition = (strncmp(line, "fn ", 3) == 0 ||
                              strncmp(line, "let ", 4) == 0 ||
                              strncmp(line, "mut ", 4) == 0 ||
                              strncmp(line, "struct ", 7) == 0);

        // Build the source to evaluate
        char *source;
        if (is_definition) {
            // Just append the definition to the environment
            size_t needed = env_len + len + 2;
            if (needed >= 65536) {
                printf("Error: Environment too large\n");
                continue;
            }
            strcat(env_code + env_len, line);
            strcat(env_code + env_len, "\n");
            env_len += len + 1;

            // Try to parse it to check for errors
            source = strdup(env_code);
        } else {
            // Wrap expression in a main function that prints the result
            // For now, just evaluate expressions that are statements
            size_t source_size = env_len + len + 256;
            source = malloc(source_size);
            snprintf(source, source_size,
                "%s"
                "fn __repl_main__() -> i64 do\n"
                "    %s\n"
                "    ret 0\n"
                "end\n",
                env_code, line);
        }

        // Try to lex and parse
        Lexer lexer;
        lexer_init(&lexer, source);

        Parser parser;
        parser_init(&parser, &lexer);
        ASTNode *ast = parser_parse(&parser);

        if (parser.had_error) {
            if (is_definition) {
                // Roll back the definition
                env_code[env_len - len - 1] = '\0';
                env_len -= len + 1;
            }
            ast_free(ast);
            lexer_free(&lexer);
            free(source);
            continue;
        }

        // Analyze
        Analyzer analyzer;
        analyzer_init(&analyzer);
        if (!analyzer_analyze(&analyzer, ast)) {
            if (is_definition) {
                env_code[env_len - len - 1] = '\0';
                env_len -= len + 1;
            }
            ast_free(ast);
            analyzer_free(&analyzer);
            lexer_free(&lexer);
            free(source);
            continue;
        }

        // If it's a definition, we're done (just checking syntax/semantics)
        if (is_definition) {
            printf("OK\n");
            ast_free(ast);
            analyzer_free(&analyzer);
            lexer_free(&lexer);
            free(source);
            line_num++;
            continue;
        }

        // For expressions, interpret them
        Interp interp;
        interp_init(&interp);
        interp_run(&interp, ast);

        interp_free(&interp);
        analyzer_free(&analyzer);
        ast_free(ast);
        lexer_free(&lexer);
        free(source);

        line_num++;
    }

    free(env_code);
    return 0;
}

static void print_usage(const char *prog) {
    printf("null - A compiled programming language\n\n");
    printf("Usage:\n");
    printf("  %s <file.null>           Run the program (compiled)\n", prog);
    printf("  %s run <file.null>       Run the program (compiled)\n", prog);
    printf("  %s interp <file.null>    Run the program (interpreted)\n", prog);
    printf("  %s repl                  Interactive interpreter\n", prog);
    printf("  %s build <file.null> -o <output>   Compile to executable\n", prog);
    printf("  %s test <dir>            Run tests in directory\n", prog);
    printf("  %s --help                Show this help\n", prog);
}

static int compile_and_run(const char *filename) {
    char *source = read_file(filename);
    if (!source) return 1;

    // Preprocess (handle imports)
    reset_imported_modules();
    char *processed = preprocess(source, filename);
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
    lexer_free(&lexer);
    free(processed);

    return result;
}

static int interpret_file(const char *filename) {
    char *source = read_file(filename);
    if (!source) return 1;

    // Preprocess (handle imports)
    reset_imported_modules();
    char *processed = preprocess(source, filename);
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

    // Interpret (no LLVM needed!)
    Interp interp;
    interp_init(&interp);
    int result = interp_run(&interp, ast);

    interp_free(&interp);
    analyzer_free(&analyzer);
    ast_free(ast);
    lexer_free(&lexer);
    free(processed);

    return result;
}

static int compile_to_executable(const char *filename, const char *output) {
    char *source = read_file(filename);
    if (!source) return 1;

    // Preprocess
    reset_imported_modules();
    char *processed = preprocess(source, filename);
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
    lexer_free(&lexer);
    free(processed);

    return result == 0 ? 0 : 1;
}

int main(int argc, char **argv) {
    // Set resource limits to prevent runaway memory/CPU usage
    set_resource_limits();
    setup_signal_handlers();

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

    if (strcmp(argv[1], "interp") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s interp <file.null>\n", argv[0]);
            return 1;
        }
        return interpret_file(argv[2]);
    }

    if (strcmp(argv[1], "repl") == 0) {
        return run_repl();
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
