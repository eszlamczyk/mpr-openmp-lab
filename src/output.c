#include "output.h"

#include <stdio.h>

void print_results(const char* prog, int n_threads, size_t n,
                   double t_random, const BucketSortTimes* times,
                   double t_total, bool sorted,
                   const char* csv_path) {
    /*
     * If csv_path is provided, append a CSV row to that file:
     *
     * Columns:
     *   algorithm, n_threads, n_elems,
     *   t_random, t_distribute, t_merge, t_sort, t_writeback, t_total,
     *   sorted
     *
     * t_merge is 0 for bs2 (no merge step).
     *
     * Otherwise, print human-readable output to stdout.
     */
    if (csv_path != NULL) {
        FILE* f = fopen(csv_path, "a");
        if (f == NULL) {
            fprintf(stderr, "Failed to open CSV file: %s\n", csv_path);
            return;
        }
        fprintf(f, "%s,%d,%zu,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%d\n",
                prog, n_threads, n,
                t_random,
                times->t_distribute,
                times->t_merge,
                times->t_sort,
                times->t_writeback,
                t_total,
                sorted ? 1 : 0);
        fclose(f);
    } else {
        printf("Threads:      %d\n",     n_threads);
        printf("Elements:     %zu\n",    n);
        printf("Sorted:       %s\n",     sorted ? "yes" : "NO");
        printf("---\n");
        printf("t_random:     %.6f s\n", t_random);
        printf("t_distribute: %.6f s\n", times->t_distribute);
        printf("t_merge:      %.6f s\n", times->t_merge);
        printf("t_sort:       %.6f s\n", times->t_sort);
        printf("t_writeback:  %.6f s\n", times->t_writeback);
        printf("t_total:      %.6f s\n", t_total);
    }
}
