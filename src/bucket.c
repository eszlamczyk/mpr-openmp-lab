#include "bucket.h"

BucketStatus bucket_push(Bucket *bucket, uint32_t elem) {
    if (bucket->n_elems == bucket->capacity) {
        return PUSH_FAILED;
    }

    bucket->elems[bucket->n_elems++] = elem;
    return PUSH_SUCCESSFUL;
}
