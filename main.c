
#include "allocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N 100000

int main(void) {
  Arena arena;
  arena_init(&arena);

  Slab slab;
  slab_init(&slab, &arena);

  printf("Slab + Arena test start\n");

  int *a = (int *)slab_alloc(&slab, sizeof(int));
  double *b = (double *)slab_alloc(&slab, sizeof(double));
  char *c = (char *)slab_alloc(&slab, 128);

  *a = 42;
  *b = 3.14159;
  strcpy(c, "hello");

  printf("a = %d\n", *a);
  printf("b = %f\n", *b);
  printf("c = %s\n", c);

  slab_free(&slab, a, sizeof(int));
  slab_free(&slab, b, sizeof(double));
  slab_free(&slab, c, 128);

  void *ptrs[N];
  size_t sizes[N];

  srand(123);

  for (size_t i = 0; i < N; i++) {
    sizes[i] = (rand() % (4 * 1024 * 1024 - 8)) + 8;
    ptrs[i] = slab_alloc(&slab, sizes[i]);
    if (!ptrs[i]) {
      fprintf(stderr, "Allocation failed at %zu\n", i);
      break;
    }
  }

  for (size_t i = 0; i < N; i++) {
    slab_free(&slab, ptrs[i], sizes[i]);
  }

  arena_destroy(&arena);

  return 0;
}
