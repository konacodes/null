#include "arena.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static ArenaBlock *arena_new_block(size_t min_size) {
    size_t size = min_size > ARENA_BLOCK_SIZE ? min_size : ARENA_BLOCK_SIZE;
    ArenaBlock *block = malloc(sizeof(ArenaBlock) + size);
    if (!block) {
        fprintf(stderr, "Arena: out of memory\n");
        exit(1);
    }
    block->next = NULL;
    block->size = size;
    block->used = 0;
    return block;
}

void arena_init(Arena *arena) {
    arena->first = NULL;
    arena->current = NULL;
}

void *arena_alloc(Arena *arena, size_t size) {
    // Align to 8 bytes
    size = (size + 7) & ~7;

    // Check if current block has space
    if (arena->current && arena->current->used + size <= arena->current->size) {
        void *ptr = arena->current->data + arena->current->used;
        arena->current->used += size;
        return ptr;
    }

    // Need a new block
    ArenaBlock *block = arena_new_block(size);
    if (arena->current) {
        arena->current->next = block;
    } else {
        arena->first = block;
    }
    arena->current = block;

    void *ptr = block->data;
    block->used = size;
    return ptr;
}

void *arena_calloc(Arena *arena, size_t count, size_t size) {
    size_t total = count * size;
    void *ptr = arena_alloc(arena, total);
    memset(ptr, 0, total);
    return ptr;
}

char *arena_strdup(Arena *arena, const char *s) {
    size_t len = strlen(s);
    char *dup = arena_alloc(arena, len + 1);
    memcpy(dup, s, len + 1);
    return dup;
}

char *arena_strndup(Arena *arena, const char *s, size_t len) {
    char *dup = arena_alloc(arena, len + 1);
    memcpy(dup, s, len);
    dup[len] = '\0';
    return dup;
}

void arena_free(Arena *arena) {
    ArenaBlock *block = arena->first;
    while (block) {
        ArenaBlock *next = block->next;
        free(block);
        block = next;
    }
    arena->first = NULL;
    arena->current = NULL;
}

void arena_reset(Arena *arena) {
    ArenaBlock *block = arena->first;
    while (block) {
        block->used = 0;
        block = block->next;
    }
    arena->current = arena->first;
}

size_t arena_total_allocated(Arena *arena) {
    size_t total = 0;
    ArenaBlock *block = arena->first;
    while (block) {
        total += block->size;
        block = block->next;
    }
    return total;
}

size_t arena_total_used(Arena *arena) {
    size_t total = 0;
    ArenaBlock *block = arena->first;
    while (block) {
        total += block->used;
        block = block->next;
    }
    return total;
}
