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


#ifdef __cplusplus
}
#endif

#endif

