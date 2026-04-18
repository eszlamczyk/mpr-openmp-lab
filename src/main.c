#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#include "bucket.h"
#include "random.h"

#define N 1000000

void print_arr(double* array, size_t n) {
    for (size_t i = 0; i < n; i++) {
        printf("%f ", array[i]);
    }
    printf("\n");
}

size_t bucket_idx(double elem) {
    return (size_t)elem;
}

int main(void) {
    double array[N];
    random_array(array, N, (double)N);
    print_arr(array, N);

    Bucket* buckets = create_buckets(N, 10);
    if (bucket_sort(array, N, buckets, N, bucket_idx) == FAILURE) {
        exit(EXIT_FAILURE);
    }
    printf("\n\n\n");

    print_arr(array, N);

    return EXIT_SUCCESS;
}
