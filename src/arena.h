#ifndef NULL_ARENA_H
#define NULL_ARENA_H

#include <stddef.h>
#include <stdbool.h>

// Arena allocator for efficient memory management
// All allocations from an arena are freed together when the arena is destroyed

#define ARENA_BLOCK_SIZE (64 * 1024)  // 64KB blocks

typedef struct ArenaBlock {
    struct ArenaBlock *next;
    size_t size;
    size_t used;
    char data[];  // Flexible array member
} ArenaBlock;

typedef struct Arena {
    ArenaBlock *first;
    ArenaBlock *current;
} Arena;

// Initialize a new arena
void arena_init(Arena *arena);

// Allocate memory from the arena (never returns NULL - exits on failure)
void *arena_alloc(Arena *arena, size_t size);

// Allocate zeroed memory from the arena
void *arena_calloc(Arena *arena, size_t count, size_t size);

// Duplicate a string into the arena
char *arena_strdup(Arena *arena, const char *s);

// Duplicate a string with length into the arena
char *arena_strndup(Arena *arena, const char *s, size_t len);

// Free all memory in the arena
void arena_free(Arena *arena);

// Reset the arena (keep blocks but mark as unused)
void arena_reset(Arena *arena);

// Get total bytes allocated
size_t arena_total_allocated(Arena *arena);

// Get total bytes used
size_t arena_total_used(Arena *arena);

#endif // NULL_ARENA_H
