/*
 * bitarray.h
 *
 * data structure for indexing single bits in 32 bit batches
 *
 * source: https://stackoverflow.com/a/24753227
 *
 */

#ifndef _BITARRAY_H_
#define _BITARRAY_H_

#include <inttypes.h> // defines uint32_t

//typedef unsigned int bitarray_t; // if you know that int is 32 bits
typedef uint32_t bitarray_t;

#define RESERVE_BITS(n) (((n)+0x1f)>>5)
#define DW_INDEX(x) ((x)>>5)
#define BIT_INDEX(x) ((x)&0x1f)
#define getbit(array,index) (((array)[DW_INDEX(index)]>>BIT_INDEX(index))&1)
#define putbit(array, index, bit) \
    ((bit)&1 ?  ((array)[DW_INDEX(index)] |= 1<<BIT_INDEX(index)) \
             :  ((array)[DW_INDEX(index)] &= ~(1<<BIT_INDEX(index))) \
             , ((void)0) \
    )

#endif /* _BITARRAY_H_ */

