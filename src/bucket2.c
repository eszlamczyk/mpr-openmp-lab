#include <omp.h>
#include <stdint.h>

void bucket_sort(uint32_t* arr, uint32_t** buckets, size_t n_elems, size_t n_buckets, size_t bucket_size) {
    omp_lock_t* locks = (omp_lock_t*)malloc(n_buckets * sizeof(omp_lock_t));

    for (size_t i = 0; i < n_buckets; i++) {
        omp_init_lock(&locks[i]);
    }

    #pragma omp parallel for
    for (size_t i = 0; i < n_elems; i++) {

    }
}

