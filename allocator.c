#include "allocator.h"

size_t get_page_aligned_size(size_t size) {
  size_t page_size = (size_t)sysconf(_SC_PAGESIZE);
  return (size + page_size - 1) & ~(page_size - 1);
}

void *get_memory(size_t size) {
  size_t aligned_size = get_page_aligned_size(size);
  void *ptr = MAP_FAILED;

#ifdef MAP_HUGETLB
  if (aligned_size >= (2 * 1024 * 1024)) {
    ptr = mmap(NULL, aligned_size, PROT_READ | PROT_WRITE,
               MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0);
  }
#endif

  if (ptr == MAP_FAILED) {
    ptr = mmap(NULL, aligned_size, PROT_READ | PROT_WRITE,
               MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  }

  if (ptr == MAP_FAILED) {
    perror("get_memory: mmap failed");
    return NULL;
  }

  return ptr;
}

void free_memory(void *ptr, size_t size) {
  if (!ptr || size == 0)
    return;

  size_t aligned_size = get_page_aligned_size(size);
  if (munmap(ptr, aligned_size) != 0) {
    perror("free_memory: munmap failed");
  }
}

void arena_init(Arena *arena) { arena->head = NULL; }

void *arena_alloc(Arena *arena, size_t size) {
  if (!arena)
    return NULL;

  size_t total_size = sizeof(Block) + size;
  Block *block = (Block *)get_memory(total_size);
  if (!block)
    return NULL;

  block->size = total_size;
  block->prev = NULL;
  block->next = arena->head;

  if (arena->head) {
    arena->head->prev = block;
  }
  arena->head = block;

  return (void *)(block + 1);
}

void arena_free(Arena *arena, void *ptr) {
  if (!arena || !ptr)
    return;

  Block *block = ((Block *)ptr) - 1;

  if (block->prev)
    block->prev->next = block->next;
  if (block->next)
    block->next->prev = block->prev;
  if (arena->head == block)
    arena->head = block->next;

  free_memory(block, block->size);
}

void arena_destroy(Arena *arena) {
  if (!arena)
    return;

  Block *curr = arena->head;
  while (curr) {
    Block *next = curr->next;
    free_memory(curr, curr->size);
    curr = next;
  }

  arena->head = NULL;
}

static const size_t classes[NUM_SLAB_CLASS] = {
    8,   16,   32,   48,   64,   80,    96,    112,   128,     192,    256,
    512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 1048576, 4194304};

size_t size_to_class(size_t size) {
  for (size_t i = 0; i < NUM_SLAB_CLASS; i++) {
    if (size <= classes[i])
      return i;
  }
  return NUM_SLAB_CLASS - 1;
}

void slab_init(Slab *slab, Arena *arena) {
  slab->arena = arena;
  memset(slab->slab_nodes, 0, sizeof(slab->slab_nodes));
}

void slab_refill(Slab *slab, size_t sc) {
  size_t block_size = classes[sc];
  size_t total_size = block_size * SLAB_PREFETCH_COUNT;

  /* Now uses arena_alloc instead of arena_alloc_raw - chunks are tracked */
  char *chunk = (char *)arena_alloc(slab->arena, total_size);
  assert(chunk != NULL);

  for (size_t i = 0; i < SLAB_PREFETCH_COUNT; i++) {
    SlabNode *node = (SlabNode *)(chunk + i * block_size);
    node->next = slab->slab_nodes[sc];
    slab->slab_nodes[sc] = node;
  }
}

void *slab_alloc(Slab *slab, size_t requested_size) {
  size_t sc = size_to_class(requested_size);

  if (!slab->slab_nodes[sc]) {
    slab_refill(slab, sc);
  }

  SlabNode *node = slab->slab_nodes[sc];
  slab->slab_nodes[sc] = node->next;
  return (void *)node;
}

void slab_free(Slab *slab, void *ptr, size_t size) {
  if (!ptr)
    return;

  size_t sc = size_to_class(size);
  SlabNode *node = (SlabNode *)ptr;

  node->next = slab->slab_nodes[sc];
  slab->slab_nodes[sc] = node;
}
