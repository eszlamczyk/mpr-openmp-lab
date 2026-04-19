#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <omp.h>

#include "bucket.h"
#include "random.h"

#define N 1000000

static size_t bucket_idx(double elem) {
    return (size_t)elem;
}

static bool is_sorted(double* array, size_t n) {
    for (size_t i = 1; i < n; i++) {
        if (array[i] <= array[i - 1]) return false;
    }
    return true;
}

int main(int argc, char* argv[]) {
    size_t n = N;
    if (argc >= 2) {
        n = (size_t)strtoul(argv[1], NULL, 10);
    }

    double* array = (double*)malloc(n * sizeof(double));
    if (array == NULL) {
        fprintf(stderr, "Failed to allocate array of size %zu\n", n);
        exit(EXIT_FAILURE);
    }

    double t_random_start = omp_get_wtime();
    random_array(array, n, (double)n);
    double t_random = omp_get_wtime() - t_random_start;

    Bucket* buckets = create_buckets(n, 10);
    if (buckets == NULL) {
        free(array);
        exit(EXIT_FAILURE);
    }

    BucketSortTimes times = {0};

    double t_total_start = omp_get_wtime();
    BucketSortStatus status = bucket_sort(array, n, buckets, n, bucket_idx, &times);
    double t_total = omp_get_wtime() - t_total_start;

    if (status == FAILURE) {
        fprintf(stderr, "bucket_sort failed\n");
        destroy_buckets(buckets, n);
        free(array);
        exit(EXIT_FAILURE);
    }

    bool sorted = is_sorted(array, n);
    if (!sorted) {
        fprintf(stderr, "RESULT IS NOT SORTED\n");
    }

#ifdef WRITE_TO_CSV
    /*
     * CSV output — one row per run, suitable for appending via slurm scripts:
     *   ./out/bsX 1000000 >> results.csv
     *
     * Columns:
     *   algorithm, n_threads, n_elems,
     *   t_random, t_distribute, t_merge, t_sort, t_writeback, t_total,
     *   sorted
     *
     * t_merge is 0 for bs2 (no merge step).
     */

    const char* prog = strrchr(argv[0], '/');
    prog = prog ? prog + 1 : argv[0];

    printf("%s,%d,%zu,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%d\n",
           prog, omp_get_max_threads(), n,
           t_random,
           times.t_distribute,
           times.t_merge,
           times.t_sort,
           times.t_writeback,
           t_total,
           sorted ? 1 : 0);
#else
    printf("Threads:      %d\n",     omp_get_max_threads());
    printf("Elements:     %zu\n",    n);
    printf("Sorted:       %s\n",     sorted ? "yes" : "NO");
    printf("---\n");
    printf("t_random:     %.6f s\n", t_random);
    printf("t_distribute: %.6f s\n", times.t_distribute);
    printf("t_merge:      %.6f s\n", times.t_merge);
    printf("t_sort:       %.6f s\n", times.t_sort);
    printf("t_writeback:  %.6f s\n", times.t_writeback);
    printf("t_total:      %.6f s\n", t_total);
#endif

    destroy_buckets(buckets, n);
    free(array);
    return sorted ? EXIT_SUCCESS : EXIT_FAILURE;
}
