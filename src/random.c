#include "random.h"
#include <omp.h>

#if defined (__APPLE__) && defined (__MACH__)

#include <stdlib.h>

void random_array(double* arr, size_t n_elems, double max) {
    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < n_elems; i++) {
        arr[i] = ((double)arc4random() / 0x100000000ULL) * max;
    }
}

#elif defined (__linux__)

#include <stdlib.h>

void random_array(double* arr, size_t n_elems, double max) {
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        unsigned short xsubi[3] = {
            (unsigned short)(tid),
            (unsigned short)(tid ^ 0x5A5A),
            (unsigned short)(tid ^ 0xA5A5)
        };

        #pragma omp for schedule(static)
        for (size_t i = 0; i < n_elems; i++) {
            arr[i] = erand48(xsubi) * max;
        }
    }
}

#endif
