#ifndef __OUTPUT__
#define __OUTPUT__

#include <stddef.h>
#include <stdbool.h>
#include "bucket.h"

void print_results(const char* prog, int n_threads, size_t n,
                   double t_random, const BucketSortTimes* times,
                   double t_total, bool sorted,
                   const char* csv_path);

#endif
