#ifndef __BUCKET__
#define __BUCKET__

#include <stddef.h>

typedef struct {
    double* elems;
    size_t n_elems;
    size_t capacity;
} Bucket;
typedef size_t (*BucketIdx)(double);

typedef enum {
    PUSH_SUCCESSFUL,
    PUSH_FAILED,
} BucketStatus;

typedef enum {
    SUCCESS,
    FAILURE,
} BucketSortStatus;

BucketStatus bucket_push(Bucket* bucket, double elem);

Bucket bucket_init(size_t capacity);
Bucket* create_buckets(size_t n, size_t capacity);

void bucket_free(Bucket* bucket);
void destroy_buckets(Bucket* buckets);

typedef struct {
    double t_distribute; /* (b)  distributing elements into buckets    */
    double t_merge;      /* (b') merging thread-local buckets (bs3 only, 0 for bs2) */
    double t_sort;       /* (c)  sorting each bucket                   */
    double t_writeback;  /* (d)  prefix-sum + copying back to array    */
} BucketSortTimes;

BucketSortStatus bucket_sort(double* array, size_t n_elems, Bucket* buckets, size_t n_buckets, BucketIdx bucket_idx, BucketSortTimes* times);

#endif
