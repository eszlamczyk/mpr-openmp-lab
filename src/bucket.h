#ifndef __BUCKET__
#define __BUCKET__

#include <stddef.h>
#include <stdint.h>

typedef struct bucket {
    uint32_t* buckets;
    size_t size;
} Bucket;

void bucket_sort(uint32_t* array, size_t n_elems, Bucket* buckets);

#endif
