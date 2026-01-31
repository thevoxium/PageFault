#include "allocator.h"
#include <sys/mman.h>

size_t get_page_aligned_size(size_t size) {
  size_t page_size = (size_t)sysconf(_SC_PAGESIZE);
  return (size_t)(size + page_size - 1) & (~(page_size - 1));
}

void *get_memory(size_t size) {
  size_t aligned_size = get_page_aligned_size(size);
  void *ptr = NULL;

#ifdef MAP_HUGETLB
  if (aligned_size >= (2 * 1024 * 1024)) {
    ptr = mmap(NULL, aligned_size, PROT_READ | PROT_WRITE,
               MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0);
  }
#endif

  if (ptr == MAP_FAILED || ptr == NULL) {
    ptr = mmap(NULL, aligned_size, PROT_READ | PROT_WRITE,
               MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  }

  if (ptr == MAP_FAILED) {
    perror("SA: get_memory -> get_memory failed");
    return NULL;
  }
  return ptr;
}

void free_memory(void *ptr, size_t size) {
  if (!ptr || size == 0) {
    return;
  }
  size_t aligned_size = get_page_aligned_size(size);
  if (munmap(ptr, aligned_size) != 0) {
    perror("SA: free_memory -> munmap failed");
  }
}

void arena_init(Arena *arena) {
  if (!arena) {
    return;
  }
  arena->head = NULL;
}

void *arena_alloc(Arena *arena, size_t size) {
  if (!arena) {
    return NULL;
  }
  size_t required_size = sizeof(Block) + size;

  Block *curr = (Block *)get_memory(required_size);
  if (!curr) {
    perror("arena_alloc: curr allocation failed");
    return NULL;
  }

  curr->size = required_size;
  curr->next = arena->head;
  curr->prev = NULL;

  if (arena->head) {
    arena->head->prev = curr;
  }
  arena->head = curr;

  return (void *)(curr + 1);
}

void arena_free(Arena *arena, void *ptr) {
  if (!arena || !ptr) {
    return;
  }

  Block *block = (Block *)ptr - 1;

  if (block->prev) {
    block->prev->next = block->next;
  }
  if (block->next) {
    block->next->prev = block->prev;
  }

  if (arena->head == block) {
    arena->head = block->next;
  }

  free_memory((void *)block, block->size);
}

void arena_destroy(Arena *arena) {
  if (!arena) {
    return;
  }

  Block *curr = arena->head;
  while (curr) {
    Block *next = curr->next;
    free_memory((void *)curr, curr->size);
    curr = next;
  }

  arena->head = NULL;
}
