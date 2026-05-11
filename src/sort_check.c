#include "sort_check.h"
#include <stdlib.h>

// compares 2 arrays, they SHOULD be the same size
bool compare_arrays(double *actual, double *expected, size_t n_elems) {
    for (size_t i = 0; i < n_elems; i++) {
        if (actual[i] != expected[i]) {
            return false;
        }
    }
    return true;
}

static int compar(const void* a, const void* b) {
    double x = *(double*)a;
    double y = *(double*)b;

    if (x > y) {
        return 1;
    } else if (x < y) {
        return -1;
    } else {
        return 0;
    }
}

// sort array of `double` with `qsort` library function
void qsort_double(double *array, size_t n_elems) {
    qsort((void*)array, n_elems, sizeof(double), compar);
}
