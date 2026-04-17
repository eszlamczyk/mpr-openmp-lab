#include <stdio.h>
#ifdef BS2

#include <stdlib.h>
#include <omp.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#include "bucket.h"

static int compare(const void* a, const void* b) {
    uint32_t x = *(uint32_t*)a;
    uint32_t y = *(uint32_t*)b;

    return x - y;
}

BucketSortStatus bucket_sort(uint32_t* array, size_t n_elems, Bucket* buckets, size_t n_buckets, BucketIdx bucket_idx) {
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
    size_t threads = (size_t)omp_get_num_threads();
    size_t chunk_size = n_buckets / threads;

    #pragma omp parallel for schedule(static, chunk_size) shared(sort_failed)
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

    #pragma omp parallel for
    for (size_t i = 0; i < n_buckets; i++) {
        omp_destroy_lock(&locks[i]);
    }
    free(locks);

    if (sort_failed) {
        return FAILURE;
    }


    #pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < n_buckets; i++) {
        qsort(buckets[i].elems, buckets[i].n_elems, sizeof(uint32_t), compare);
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
        memcpy(&array[write_at[i]], buckets[i].elems, buckets[i].n_elems * sizeof(uint32_t));
    }
    free(write_at);

    return SUCCESS;
}


#endif
