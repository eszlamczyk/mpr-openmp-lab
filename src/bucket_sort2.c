#include <stdio.h>
#ifdef BS2

#include <stdlib.h>
#include <omp.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#include "bucket.h"

static int compare(const void* a, const void* b) {
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

BucketSortStatus bucket_sort(double* array, size_t n_elems, Bucket* buckets, size_t n_buckets, BucketIdx bucket_idx, BucketSortTimes* times) {
    omp_lock_t* locks = (omp_lock_t*)malloc(n_buckets * sizeof(omp_lock_t));
    if (locks == NULL) {
        fprintf(stderr, "malloc failed miserably\n");
        exit(EXIT_FAILURE);
    }

    #pragma omp parallel for
    for (size_t i = 0; i < n_buckets; i++) {
        omp_init_lock(&locks[i]);
    }

    bool sort_failed = false;

    double t_start_distribute = omp_get_wtime();

    #pragma omp parallel for schedule(static) shared(sort_failed)
    for (size_t i = 0; i < n_elems; i++) {
        size_t idx = bucket_idx(array[i]);

        omp_set_lock(&locks[idx]);
        switch (bucket_push(&buckets[idx], array[i])) {
            case PUSH_SUCCESSFUL: break;
            case PUSH_FAILED:
                #pragma omp atomic write
                sort_failed = true;
                break;
        }
        omp_unset_lock(&locks[idx]);
    }

    times->t_distribute = omp_get_wtime() - t_start_distribute;
    times->t_merge = 0.0;

    #pragma omp parallel for
    for (size_t i = 0; i < n_buckets; i++) {
        omp_destroy_lock(&locks[i]);
    }
    free(locks);

    if (sort_failed) {
        return FAILURE;
    }

    double t_start_sort = omp_get_wtime();

    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < n_buckets; i++) {
        qsort(buckets[i].elems, buckets[i].n_elems, sizeof(double), compare);
    }

    times->t_sort = omp_get_wtime() - t_start_sort;

    double t_start_writeback = omp_get_wtime();

    size_t* write_at = (size_t*)malloc(n_buckets * sizeof(size_t));
    if (write_at == NULL) {
        fprintf(stderr, "malloc failed miserably\n");
        exit(EXIT_FAILURE);
    }
    write_at[0] = 0;
    for (size_t i = 1; i < n_buckets; i++) {
        write_at[i] = write_at[i - 1] + buckets[i - 1].n_elems;
    }

    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < n_buckets; i++) {
        memcpy(&array[write_at[i]], buckets[i].elems, buckets[i].n_elems * sizeof(double));
    }
    free(write_at);

    times->t_writeback = omp_get_wtime() - t_start_writeback;

    return SUCCESS;
}


#endif
