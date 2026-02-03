// I was a bit lazy to write the main.c by myself, this is written with the help
// of kimi 2.5 thinking model; do not trust the test cases here, however it runs
// fine

#include "allocator.h"
#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define NUM_THREADS 4
#define ALLOCS_PER_THREAD 10000

/* Per-allocation tracking for tests that need sized free */
typedef struct {
  void *ptr;
  size_t size;
} AllocTrack;

/* Single-threaded correctness test */
void test_basic() {
  printf("=== Basic Single-Threaded Test ===\n");

  tc_init();

  /* Test various size classes */
  void *ptrs[10];
  size_t sizes[] = {8, 16, 32, 64, 128, 256, 512, 1024, 4096, 8192};

  for (int i = 0; i < 10; i++) {
    ptrs[i] = tc_alloc(sizes[i]);
    if (!ptrs[i]) {
      fprintf(stderr, "Failed to allocate %zu bytes\n", sizes[i]);
      exit(1);
    }
    /* Write to verify memory is writable */
    memset(ptrs[i], 0xAB, sizes[i]);
    printf("Allocated %zu bytes at %p\n", sizes[i], ptrs[i]);
  }

  /* Free in reverse order to test list handling */
  for (int i = 9; i >= 0; i--) {
    tc_free(ptrs[i], sizes[i]);
    printf("Freed %zu bytes\n", sizes[i]);
  }

  printf("Basic test passed!\n\n");
}

/* Thread-local cache test - each thread allocates and frees independently */
void *thread_work(void *arg) {
  int tid = *(int *)arg;
  void *ptrs[32]; /* Exactly one batch size */
  int magic = 0xDEADBEEF + tid;

  printf("Thread %d: Starting allocations\n", tid);

  /* Allocate exactly 32 items to fill cache */
  for (int i = 0; i < 32; i++) {
    ptrs[i] = tc_alloc(64); /* size class 4 (64 bytes) */
    if (!ptrs[i]) {
      fprintf(stderr, "Thread %d: Allocation failed\n", tid);
      return NULL;
    }
    *(int *)ptrs[i] = magic; /* Write unique pattern */
  }

  printf("Thread %d: Filled local cache (32 items)\n", tid);

  /* Free all - this should trigger flush of 16 to global when cache overflows
   */
  for (int i = 0; i < 32; i++) {
    tc_free(ptrs[i], 64);
  }

  printf("Thread %d: Freed all\n", tid);

  /* Allocate again - should work without crash even if we get global memory */
  for (int i = 0; i < 16; i++) {
    void *p = tc_alloc(64);
    if (!p) {
      fprintf(stderr, "Thread %d: Reallocation failed\n", tid);
      return NULL;
    }
    *(int *)p = tid; /* Write our tid */
    tc_free(p, 64);
  }

  printf("Thread %d: Completed successfully\n", tid);
  return NULL;
}

void test_multithreaded() {
  printf("=== Multi-Threaded Cache Test ===\n");

  pthread_t threads[NUM_THREADS];
  int tids[NUM_THREADS];

  for (int i = 0; i < NUM_THREADS; i++) {
    tids[i] = i;
    pthread_create(&threads[i], NULL, thread_work, &tids[i]);
  }

  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i], NULL);
  }

  printf("Multi-threaded test passed!\n\n");
}

/* Stress test: Heavy contention with correct size tracking */
void *stress_worker(void *arg) {
  int tid = *(int *)arg;
  AllocTrack tracks[100];
  size_t sizes[] = {8, 16, 32, 64, 128};

  for (int round = 0; round < 100; round++) {
    /* Allocate mixed sizes */
    for (int i = 0; i < 100; i++) {
      size_t sz = sizes[rand() % 5];
      tracks[i].size = sz;
      tracks[i].ptr = tc_alloc(sz);
      if (tracks[i].ptr)
        memset(tracks[i].ptr, tid, sz);
    }

    /* Free all with CORRECT sizes */
    for (int i = 0; i < 100; i++) {
      tc_free(tracks[i].ptr, tracks[i].size);
    }
  }

  return NULL;
}

void test_stress() {
  printf("=== Stress Test (4 threads x 100 rounds x 100 allocs) ===\n");

  clock_t start = clock();

  pthread_t threads[4];
  int tids[4] = {0, 1, 2, 3};

  for (int i = 0; i < 4; i++) {
    pthread_create(&threads[i], NULL, stress_worker, &tids[i]);
  }

  for (int i = 0; i < 4; i++) {
    pthread_join(threads[i], NULL);
  }

  clock_t end = clock();
  double cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;

  printf("Stress test completed in %.3f seconds\n", cpu_time);
  printf("Total allocations: %d\n", 4 * 100 * 100);
  printf("Allocs/sec: %.0f\n\n", (4 * 100 * 100) / cpu_time);
}

int main() {
  srand(time(NULL));

  printf("Thread-Caching Allocator Test Suite\n");
  printf("===================================\n\n");

  test_basic();
  test_multithreaded();
  test_stress();

  printf("All tests passed!\n");
  return 0;
}
