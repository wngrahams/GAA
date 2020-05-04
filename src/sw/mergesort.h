/*
 * mergesort.h
 */

#ifndef _MERGESORT_H_
#define _MERGESORT_H_

#define INSERTION_SORT_THRESHOLD 7  // max number of individuals to insertion
                                    // sort instead of mergesort

#include "ga-params.h"
#include "ga-utils.h"


static inline void _merge_idv(int* arr, int l, int m, int r, Individual* pop) {
    
    int i, j, k;
    int n1, n2;
    int *left, *right;

    n1 = m-l+1;
    n2 = r-m;

    // temp arrays:
    left = malloc(n1 * sizeof(int));
    CHECK_MALLOC_ERR(left);
    right = malloc(n2 * sizeof(int));
    CHECK_MALLOC_ERR(right);

    // copy data into temp arrays
    for (i=0; i<n1; i++) {
        left[i] = arr[l + i];
    }
    for (j=0; j<n2; j++) {
        right[j] = arr[m + 1 + j];
    }

    // merge temp arrays back into main arrays
    i = 0;
    j = 0;
    k = l;
    while (i < n1 && j < n2) {
        if (pop[left[i]].fitness <= pop[right[j]].fitness) {
            arr[k] = left[i];
            i++;
        }
        else {
            arr[k] = right[j];
            j++;
        }
        k++;
    }

    // copy anything remaining in left array
    while (i < n1) {
        arr[k] = left[i];
        i++;
        k++;
    }

    // copy anything remaining in right array
    while (j < n2) {
        arr[k] = right[j];
        j++;
        k++;
    }

    // free temp arrays
    free(left);
    free(right);
}


/*
 * Sort an array of integers according to their corresponding fitness values
 * using insertion sort.
 */
static inline void insertionsort_idv(int* arr, int l, int r, Individual* pop) {

    int i, j, temp;
    double key;

    for (i=l+1; i<r; i++) {
        temp = arr[i];
        key = pop[arr[i]].fitness;
        j = i-1;

        while (j >= l && pop[arr[j]].fitness  > key) {
            arr[j+1] = arr[j];
            j--;
        }
        arr[j+1] = temp;
    }
}


/* 
 * Sort an array of integers according to the corresponding fitnesses of the
 * passed array of Individuals. Result is an array of indexes from most fit
 * (lowest fitness value) to least fit (highest fitness value) individual.
 */
static inline void mergesort_idv(int* arr, int l, int r, Individual* pop) {

    int mid;
    
    if (l < r) {
        if ((r - l + 1) <= INSERTION_SORT_THRESHOLD) {
            insertionsort_idv(arr, l, r+1, pop);
            return;
        }
        mid = (l+r)/2;  // not overflow safe (TODO)

        // recursively sort
        mergesort_idv(arr, l, mid, pop);
        mergesort_idv(arr, mid+1, r, pop);

        // merge
        _merge_idv(arr, l, mid, r, pop);
    }
}

#endif /* _MERGESORT_H_ */

