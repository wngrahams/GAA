/*
 * ga-utils.h
 *
 */

#ifndef _GA_UTILS_
#define _GA_UTILS_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define F_PI 3.141592653f

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __GNUC__
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif

#define MAX(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })
#define MIN(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })
#define MOD(a,b) ((((a)%(b))+(b))%(b))


#define CHECK_MALLOC_ERR(ptr) \
        (unlikely(!check_malloc_err(ptr)) ? (int_exit(1)) : (1))

static inline int int_exit(int x) { exit(x); return x; }


/*
 * This function checks if malloc() returned NULL. If it did, the program
 * prints an error message. The function returns 1 on success and 0 on failure
 */
static inline int8_t check_malloc_err(const void *ptr) {
    if (NULL == ptr) {
        perror("malloc() returned NULL");
        return 0;
    } /* END if */

    return 1;
}


/*
 * gets the sign of an integer with no branch instructions
 * returns -1, 0, or 1
 */
static inline int get_sign(int x) { return (x > 0) - (x < 0); }


/*
 * function to calculate the hamming distance between two ints
 */
#ifdef __GNUC__
#define hamming_distance(x,y) _popcount_hamming_distance((x),(y))
#else
#define hamming_distance(x,y) _wegner_hamming_distance((x),(y))
#endif
static inline int _popcount_hamming_distance(unsigned x, unsigned y) {
    return __builtin_popcount(x ^ y);
}
static inline int _wegner_hamming_distance(unsigned x, unsigned y) {
    int dist = 0;
    
    // Count the number of bits set
    for (unsigned val = x ^ y; val > 0; val = val >> 1)
    {
        // If A bit is set, so increment the count
        if (val & 1)
            dist++;
        // Clear (delete) val's lowest-order bit
    }

    // Return the number of differing bits
    return dist;
}


/* Returns an integer in the range [0, n) from a uniform distribution.
 *
 * Uses rand(), and so is affected-by/affects the same seed.
 */
static inline int urandint(int n) {
  if ((n - 1) == RAND_MAX) {
    return rand();
  } else {
    // Supporting larger values for n would requires an even more
    // elaborate implementation that combines multiple calls to rand()
    assert (n <= RAND_MAX);

    // Chop off all of the values that would cause skew...
    int end = RAND_MAX / n; // truncate skew
    assert (end > 0);
    end *= n;

    // ... and ignore results from rand() that fall above that limit.
    // (Worst case the loop condition should succeed 50% of the time,
    // so we can expect to bail out of this loop pretty quickly.)
    int r;
    while ((r = rand()) >= end);

    return r % n;
  }
}


#ifdef __cplusplus
}
#endif

#endif

