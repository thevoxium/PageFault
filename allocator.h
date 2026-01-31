#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

void *get_memory(size_t size);
void free_memory(void *ptr, size_t size);
size_t get_page_aligned_size(size_t size);

typedef struct Block {
  size_t size;
  struct Block *prev;
  struct Block *next;
} Block;

typedef struct Arena {
  Block *head;
} Arena;

void arena_init(Arena *arena);
void *arena_alloc(Arena *arena, size_t size);
void arena_free(Arena *arena, void *ptr);
void arena_destroy(Arena *arena);

#endif
