/*
 * dynamic_array.c
 *
 * A dynamic array implemented with the "shadow header" trick:
 * the bookkeeping info (length/capacity) is stored in a small
 * struct placed IMMEDIATELY BEFORE the user-visible data pointer.
 *
 *   [ArrayHeader][elem0][elem1][elem2]...
 *                 ^
 *                 |
 *          this is the pointer the user actually holds
 *
 * This means the "array" is just a normal T* pointer that you can
 * index (arr[i]), pass to functions, sizeof-check elements of, etc.
 * The header is recovered by stepping backwards from the pointer.
 *
 * Usage:
 *
 *     int *nums = NULL;
 *     push_array(nums, 10);
 *     push_array(nums, 20);
 *     push_array(nums, 30);
 *
 *     for (size_t i = 0; i < array_length(nums); i++)
 *         printf("%d\n", nums[i]);
 *
 *     array_free(nums);
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* The shadow header                                                   */
/* ------------------------------------------------------------------ */
typedef struct {
    size_t length;
    size_t capacity;
} ArrayHeader;

/* Recover the header from a user-facing array pointer. */
#define array_header(a) ((ArrayHeader *)(a) - 1)

/* Safe accessors: work even when the array pointer is NULL. */
#define array_length(a)   ((a) ? array_header(a)->length   : (size_t)0)
#define array_capacity(a)  ((a) ? array_header(a)->capacity : (size_t)0)

/* ------------------------------------------------------------------ */
/* Growth                                                              */
/* ------------------------------------------------------------------ */

/*
 * Reallocates the block so that it can hold at least `min_capacity`
 * elements of size `elem_size`, and returns the (possibly moved)
 * user-facing pointer (i.e. pointing just past the header).
 */
static void *array_grow_impl(void *arr, size_t min_capacity, size_t elem_size) {
    ArrayHeader *new_header;
    size_t new_capacity;

    if (arr) {
        ArrayHeader *header = array_header(arr);
        new_capacity = header->capacity * 2;
        if (new_capacity < min_capacity) {
            new_capacity = min_capacity;
        }
        new_header = (ArrayHeader *)realloc(header,
                        sizeof(ArrayHeader) + new_capacity * elem_size);
    } else {
        new_capacity = (min_capacity > 4) ? min_capacity : 4;
        new_header = (ArrayHeader *)malloc(
                        sizeof(ArrayHeader) + new_capacity * elem_size);
        if (new_header) {
            new_header->length = 0;
        }
    }

    if (!new_header) {
        fprintf(stderr, "dynamic_array: out of memory\n");
        exit(1);
    }

    new_header->capacity = new_capacity;
    return (void *)(new_header + 1);
}

/* Ensures capacity for at least `n` elements, growing/reallocating if needed. */
#define array_grow(a, n) \
    ((a) = array_grow_impl((a), (n), sizeof(*(a))))

/* ------------------------------------------------------------------ */
/* The macro the task asked for                                        */
/* ------------------------------------------------------------------ */

/*
 * push_array(a, item)
 *
 * Appends `item` to the end of array `a`, growing the backing
 * allocation (and updating `a` itself, since realloc may move it)
 * if there isn't enough capacity left.
 *
 * `a` must be an lvalue of pointer type (e.g. `int *nums = NULL;`).
 */
#define push_array(a, item)                                             \
    do {                                                                \
        if (!(a) || array_header(a)->length >= array_header(a)->capacity) { \
            array_grow((a), array_length(a) + 1);                       \
        }                                                                \
        (a)[array_header(a)->length++] = (item);                        \
    } while (0)

/* ------------------------------------------------------------------ */
/* Small extra conveniences (not required, but make the array usable)  */
/* ------------------------------------------------------------------ */

/* Frees the array (including its hidden header) and nulls the pointer. */
#define array_free(a)                 \
    do {                              \
        if (a) {                      \
            free(array_header(a));    \
            (a) = NULL;                \
        }                              \
    } while (0)

/* Removes all elements without freeing the backing memory. */
#define array_clear(a) \
    do { if (a) array_header(a)->length = 0; } while (0)

/* ------------------------------------------------------------------ */
/* Demo / sanity test                                                   */
/* ------------------------------------------------------------------ */
#ifdef DYNAMIC_ARRAY_DEMO
typedef struct {
    int x, y;
} Point;

int main(void) {
    /* --- int array --- */
    int *nums = NULL;
    for (int i = 0; i < 10; i++) {
        push_array(nums, i * i);
    }

    printf("nums: length=%zu capacity=%zu\n",
           array_length(nums), array_capacity(nums));
    for (size_t i = 0; i < array_length(nums); i++) {
        printf("  nums[%zu] = %d\n", i, nums[i]);
    }
    array_free(nums);
    printf("after free, nums = %p, length = %zu\n\n",
           (void *)nums, array_length(nums));

    /* --- struct array, to show it works with any element type --- */
    Point *pts = NULL;
    push_array(pts, ((Point){1, 2}));
    push_array(pts, ((Point){3, 4}));
    push_array(pts, ((Point){5, 6}));

    printf("pts: length=%zu capacity=%zu\n",
           array_length(pts), array_capacity(pts));
    for (size_t i = 0; i < array_length(pts); i++) {
        printf("  pts[%zu] = (%d, %d)\n", i, pts[i].x, pts[i].y);
    }
    array_free(pts);

    return 0;
}
#endif