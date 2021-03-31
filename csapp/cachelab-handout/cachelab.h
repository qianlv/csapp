/* 
 * cachelab.h - Prototypes for Cache Lab helper functions
 */

#ifndef CACHELAB_TOOLS_H
#define CACHELAB_TOOLS_H

#define MAX_TRANS_FUNCS 100

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct trans_func{
  void (*func_ptr)(int M,int N,int[N][M],int[M][N]);
  char* description;
  char correct;
  unsigned int num_hits;
  unsigned int num_misses;
  unsigned int num_evictions;
} trans_func_t;

enum CacheResulType {
    Hit,
    Missing,
    Eviction
};

typedef struct cache_config {
    int s;
    int E;
    int b;
} cache_config_t;

typedef struct cache_line {
    bool valid;
    uint64_t tag;
    uint64_t lru;
} cache_line_t;

typedef struct cache_set {
    cache_line_t* lines;
    uint64_t size;
} cache_set_t;

typedef struct cache {
    cache_set_t* sets;
    uint64_t size;
} cache_t;

/* 
 * printSummary - This function provides a standard way for your cache
 * simulator * to display its final hit and miss statistics
 */ 
void printSummary(int hits,  /* number of  hits */
				  int misses, /* number of misses */
				  int evictions); /* number of evictions */

/* Fill the matrix with data */
void initMatrix(int M, int N, int A[N][M], int B[M][N]);

/* The baseline trans function that produces correct results. */
void correctTrans(int M, int N, int A[N][M], int B[M][N]);

/* Add the given function to the function list */
void registerTransFunction(
    void (*trans)(int M,int N,int[N][M],int[M][N]), char* desc);

#endif /* CACHELAB_TOOLS_H */
