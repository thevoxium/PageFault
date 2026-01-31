#include "allocator.h"
#include <stdio.h>

int main(void) {
  Arena arena;
  arena_init(&arena);

  int *a = (int *)arena_alloc(&arena, sizeof(int));
  double *b = (double *)arena_alloc(&arena, sizeof(double));
  char *c = (char *)arena_alloc(&arena, 128);

  if (!a || !b || !c) {
    fprintf(stderr, "Allocation failed\n");
    arena_destroy(&arena);
    return 1;
  }

  *a = 42;
  *b = 3.14159;

  printf("a = %d\n", *a);
  printf("b = %f\n", *b);
  printf("c = %s\n", c);

  arena_free(&arena, b);
  arena_destroy(&arena);
  return 0;
}
