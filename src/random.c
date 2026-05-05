#include "random.h"
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>
#include <sys/random.h>

void random_array(double* arr, size_t n_elems, double max) {
    #pragma omp parallel
    {
        unsigned short xsubi[3];
        getentropy(xsubi, sizeof(xsubi));

        #pragma omp for schedule(static)
        for (size_t i = 0; i < n_elems; i++) {
            arr[i] = erand48(xsubi) * max;
        }
    }
}

