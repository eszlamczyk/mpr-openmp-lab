#ifndef __BUCKET__
#define __BUCKET__

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t* elems;
    size_t n_elems;
    size_t capacity;
} Bucket;
typedef size_t (*BucketIdx)(uint32_t);

typedef enum {
    PUSH_SUCCESSFUL,
    PUSH_FAILED,
} BucketStatus;

typedef enum {
    SUCCESS,
    FAILURE,
} BucketSortStatus;

BucketStatus bucket_push(Bucket* bucket, uint32_t elem);

Bucket bucket_init(size_t capacity);
Bucket* create_buckets(size_t n, size_t capacity);

void bucket_free(Bucket* bucket);
void destroy_buckets(Bucket* buckets, size_t n);

BucketSortStatus bucket_sort(uint32_t* array, size_t n_elems, Bucket* buckets, size_t n_buckets, BucketIdx bucket_idx);

#endif
