#ifndef _SORT_CHECK
#define _SORT_CHECK

#include <stdbool.h>
#include <stddef.h>

bool compare_arrays(double* actual, double* expected, size_t n);

void qsort_double(double* array, size_t n_elems);

#endif
