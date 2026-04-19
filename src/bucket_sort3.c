#ifdef BS3
#include "bucket.h"
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

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

BucketSortStatus bucket_sort(double *array, size_t n_elems, Bucket *buckets, size_t n_buckets, BucketIdx bucket_idx) {
    size_t n_threads = (size_t)omp_get_max_threads();

    /*
     * Each thread holds a private copy of all buckets.
     * With a uniform distribution, each thread-local bucket holds ~n / (n_threads * n_buckets) elements,
     * so a small initial capacity suffices and total memory stays O(n) rather than O(n_threads * n).
     */
    size_t local_capacity = 10;
    Bucket* thread_buckets = create_buckets(n_threads * n_buckets, local_capacity);

    bool sort_failed = false;

    #pragma omp parallel shared(sort_failed)
    {
        size_t tid = (size_t)omp_get_thread_num();
        size_t chunk = n_elems / n_threads;
        size_t start = tid * chunk;
        size_t end = (tid == n_threads - 1) ? n_elems : start + chunk;

        for (size_t i = start; i < end; i++) {
            size_t idx    = bucket_idx(array[i]);
            Bucket* local = &thread_buckets[tid * n_buckets + idx];

            switch (bucket_push(local, array[i])) {
                case PUSH_SUCCESSFUL: break;
                case PUSH_FAILED:
                    #pragma omp atomic write
                    sort_failed = true;
                    break;
            }
        }
    }

    if (sort_failed) {
        destroy_buckets(thread_buckets, n_threads * n_buckets);
        return FAILURE;
    }

    #pragma omp parallel for schedule(dynamic) shared(sort_failed)
    for (size_t b = 0; b < n_buckets; b++) {
        for (size_t t = 0; t < n_threads; t++) {
            Bucket* src = &thread_buckets[t * n_buckets + b];
            for (size_t i = 0; i < src->n_elems; i++) {
                switch (bucket_push(&buckets[b], src->elems[i])) {
                    case PUSH_SUCCESSFUL: break;
                    case PUSH_FAILED:
                        #pragma omp atomic write
                        sort_failed = true;
                        break;
                }
            }
        }
    }

    destroy_buckets(thread_buckets, n_threads * n_buckets);

    if (sort_failed) {
        return FAILURE;
    }

    #pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < n_buckets; i++) {
        qsort(buckets[i].elems, buckets[i].n_elems, sizeof(double), compare);
    }

    size_t* write_at = (size_t*)malloc(n_buckets * sizeof(size_t));
    if (write_at == NULL) {
        fprintf(stderr, "malloc failed miserably\n");
        exit(EXIT_FAILURE);
    }
    write_at[0] = 0;
    for (size_t i = 1; i < n_buckets; i++) {
        write_at[i] = write_at[i - 1] + buckets[i - 1].n_elems;
    }

    #pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < n_buckets; i++) {
        memcpy(&array[write_at[i]], buckets[i].elems, buckets[i].n_elems * sizeof(double));
    }
    free(write_at);

    return SUCCESS;
}

#endif
