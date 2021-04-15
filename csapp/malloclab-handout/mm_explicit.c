/*
 * mm-implicit.c - implicit free list
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Qianlv",
    /* First member's full name */
    "Qianlv",
    /* First member's email address */
    "qianlv7@qq.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

#define WSIZE       4
#define DSIZE       8
#define CHUNKSIZE   (1 << 12)

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PACK(size, alloc)   ((size) | (alloc))

#define GET(p)          (*(unsigned int*)(p))
#define PUT(p, val)     (*(unsigned int*)(p) = (val))

#define GET_SIZE(p)     (GET(p) & ~0x7)
#define GET_ALLOC(p)    (GET(p) & 0x1)

#define HDRP(bp)        ((char*)(bp) - WSIZE)
#define FTRP(bp)        ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp)   ((char*)(bp) + GET_SIZE(((char*)(bp) - WSIZE)))
#define PREV_BLKP(bp)   ((char*)(bp) - GET_SIZE(((char*)(bp) - DSIZE)))    


typedef struct node {
    struct node *prev;
    struct node *next;
} node_t;
static void *heap_list = NULL;
static node_t *free_list = NULL;

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

static void remove_list(void *bp) {
    node_t *curr = bp;
    node_t *prev = curr->prev;
    node_t *next = curr->next;
    if (prev == NULL) {
        free_list = next;
    } else {
        prev->next = next;
    }

    if (next) {
        next->prev = prev;
    }
}

static void add_list(void *bp) {
    node_t *curr = bp;
    curr->next = curr->prev = NULL;
    if (free_list != NULL) {
        curr->next = free_list;
        free_list->prev = curr;
    }
    free_list = curr;
}

static void *coalesce(void *bp) {
    void *prev = PREV_BLKP(bp);
    void *next = NEXT_BLKP(bp);
    size_t prev_alloc = GET_ALLOC(HDRP(prev));
    size_t next_alloc = GET_ALLOC(HDRP(next));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {
        /* return bp; */
    } else if (prev_alloc && !next_alloc) {
        remove_list(next);
        size += GET_SIZE(HDRP(next));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) {
        remove_list(prev);
        size += GET_SIZE(FTRP(prev));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(prev), PACK(size, 0));
        bp = prev;
    } else {
        remove_list(prev);
        remove_list(next);
        size += GET_SIZE(HDRP(prev));
        size += GET_SIZE(HDRP(next));
        PUT(HDRP(prev), PACK(size, 0));
        PUT(FTRP(next), PACK(size, 0));
        bp = prev;
    }

    add_list(bp);
    return bp;
}

static void *extend_heap(size_t words) {
    char *bp;
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1) {
        return NULL;
    }

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_list = mem_sbrk(4 * WSIZE)) == (void*)-1) {
        return -1;
    }

    PUT(heap_list, 0);
    PUT(heap_list + (1 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_list + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_list + (3 * WSIZE), PACK(0, 1));

    heap_list += (2 * WSIZE);
    free_list = NULL;

    if (extend_heap(4) == NULL) {
        return -1;
    }

    return 0;
}

static void *find_fit_first(size_t asize) {
    size_t alloc;
    size_t size;
    char *bp = NULL;

    node_t *curr = free_list;
    while (curr) {
        bp = (char*)curr;
        alloc = GET_ALLOC(HDRP(bp));
        size = GET_SIZE(HDRP(bp));
        if (size == 0) {
            break;
        }

        if (!alloc && size >= asize) {
            return bp;
        }
        /* printf("%p %p\n", curr, curr->next); */
        curr = curr->next;
    }
    return NULL;
}

static void *find_fit_best(size_t asize) {
    size_t alloc;
    size_t size;
    size_t best_diff = 0;
    char *best_bp = NULL;

    char *bp;

    bp = heap_list;
    while (1) {
        alloc = GET_ALLOC(HDRP(bp));
        size = GET_SIZE(HDRP(bp));
        if (size == 0) {
            break;
        }

        if (!alloc && size >= asize) {
            if (best_bp == NULL || best_diff > (size - asize)) {
                best_bp = bp;
                best_diff = size - asize;
            }
        }

        bp = NEXT_BLKP(bp);
    }
    return best_bp;
}

static void place(void *bp, size_t asize) {
    size_t size = GET_SIZE(HDRP(bp));

    remove_list(bp);

    if (size > asize + 2 * DSIZE) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        size_t left_size = size - asize;
        char *nbp = NEXT_BLKP(bp);
        PUT(HDRP(nbp), PACK(left_size, 0));
        PUT(FTRP(nbp), PACK(left_size, 0));
        add_list(nbp);
    } else {
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
    }
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
    size_t asize;
    size_t extendsize;
    char *bp;

    if (size == 0) {
        return NULL;
    }

    if (size <= DSIZE) {
        asize = 2 * DSIZE;
    } else {
        asize = DSIZE * ((size + DSIZE + DSIZE - 1) / DSIZE);
    }

    /* printf("mm_malloc %ld\n", size); */
    if ((bp = find_fit_first(asize)) != NULL) {
        place(bp, asize);
        /* printf("end mm_malloc %ld\n", size); */
        return bp;
    }
    /* printf("---\n"); */

    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL) {
        return NULL;
    }
    place(bp, asize);
    /* printf("end mm_malloc %ld\n", size); */
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp) {
    if (!bp) {
        return;
    }

    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
    /* printf("mm_free\n"); */
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    if (ptr == NULL) {
        return mm_malloc(size);
    }

    copySize = GET_SIZE(HDRP(oldptr)) - DSIZE; 
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    if (size < copySize) {
        copySize = size;
    }
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
