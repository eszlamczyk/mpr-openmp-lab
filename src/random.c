#include "random.h"
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>
#include <sys/random.h>

void random_array(double* arr, size_t n_elems, double max) {
    size_t threads = (size_t)omp_get_max_threads();
    const size_t chunk_size = n_elems / threads;
    #pragma omp parallel
    {
        unsigned short xsubi[3];
        getentropy(xsubi, sizeof(xsubi));

        #pragma omp for schedule(static, chunk_size)
        for (size_t i = 0; i < n_elems; i++) {
            arr[i] = erand48(xsubi) * max;
        }
    }
}

