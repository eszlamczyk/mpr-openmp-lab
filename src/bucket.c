#include "bucket.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

Bucket bucket_init(size_t capacity) {
    double* array = (double*)malloc(capacity * sizeof(double));
    if (array == NULL) {
        fprintf(stderr, "malloc failed miserably");
        exit(EXIT_FAILURE);
    }

    Bucket bucket = {
        .elems = array,
        .n_elems = 0,
        .capacity = capacity,
    };
    return bucket;
}

void bucket_free(Bucket *bucket) {
    free(bucket->elems);
}

Bucket* create_buckets(size_t n, size_t capacity) {
    void* block = malloc(n * sizeof(Bucket) + n * capacity * sizeof(double));
    if (block == NULL) {
        fprintf(stderr, "malloc failed miserably");
        exit(EXIT_FAILURE);
    }

    Bucket* buckets = (Bucket*)block;
    double* slab    = (double*)((char*)block + n * sizeof(Bucket));

    for (size_t i = 0; i < n; i++) {
        buckets[i].elems    = &slab[i * capacity];
        buckets[i].n_elems  = 0;
        buckets[i].capacity = capacity;
    }
    return buckets;
}

void destroy_buckets(Bucket *buckets) {
    free(buckets);
}

BucketStatus bucket_push(Bucket *bucket, double elem) {
    if (bucket->n_elems == bucket->capacity) {
        return PUSH_FAILED;
    }

    bucket->elems[bucket->n_elems++] = elem;
    return PUSH_SUCCESSFUL;
}
