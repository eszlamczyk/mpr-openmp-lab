#include "random.h"
#include <stdint.h>


#if defined (__APPLE__) && defined (__MACH__)

#include <stdlib.h>

void random_array(uint32_t* arr, size_t n_elems) {
    for (size_t i = 0; i < n_elems; i++) {
        arr[i] = arc4random();
    }
}

#elif defined (__linux__)

// TODO

#endif
