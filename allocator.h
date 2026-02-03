#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <assert.h>
#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define NUM_SLAB_CLASS 21
#define SLAB_PREFETCH_COUNT 64

typedef struct CacheLine {
  void *list_head;
  uint16_t count;
  uint16_t batch_size;
  char padding[52];
} __attribute__((aligned(64))) CacheLine;

typedef struct ThreadCache {
  CacheLine lines[NUM_SLAB_CLASS];
} ThreadCache;

typedef struct Block {
  size_t size;
  struct Block *prev;
  struct Block *next;
} Block;

typedef struct Arena {
  Block *head;
} Arena;

typedef struct SlabNode {
  struct SlabNode *next;
} SlabNode;

typedef struct Slab {
  SlabNode *slab_nodes[NUM_SLAB_CLASS];
  Arena *arena;
  pthread_mutex_t mutex;
} Slab;

void *get_memory(size_t size);
void free_memory(void *ptr, size_t size);
size_t get_page_aligned_size(size_t size);

void arena_init(Arena *arena);
void *arena_alloc(Arena *arena, size_t size);
void arena_free(Arena *arena, void *ptr);
void arena_destroy(Arena *arena);

size_t size_to_class(size_t size);
void slab_init(Slab *slab, Arena *arena);
void slab_refill(Slab *slab, size_t sc);
void *slab_alloc(Slab *slab, size_t size);
void slab_free(Slab *slab, void *ptr, size_t size);

void tc_init(void);
void *tc_alloc(size_t size);
void tc_free(void *ptr, size_t size);

#endif /* ALLOCATOR_H */
