#include "random.h"
#include <stdint.h>
#include <omp.h>

#if defined (__APPLE__) && defined (__MACH__)

#include <stdlib.h>

void random_array(uint32_t* arr, size_t n_elems, uint32_t max) {
    size_t threads = omp_get_num_threads();
    size_t chunk_size = n_elems / threads;

    #pragma omp parallel for schedule(static, chunk_size)
    for (size_t i = 0; i < n_elems; i++) {
        arr[i] = arc4random() % max;
    }
}

#elif defined (__linux__)

// TODO

#endif
