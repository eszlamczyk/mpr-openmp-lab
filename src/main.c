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

int main(int argc, char* argv[]) {
    size_t n = N;
    if (argc == 2) {
        n = (size_t)strtoul(argv[1], NULL, 10);
    }
    double* array = (double*)malloc(n * sizeof(double));
    if (array == NULL) {
        fprintf(stderr, "Failed to allocate array of size %zu\n", n);
        exit(EXIT_FAILURE);
    }
    random_array(array, n, (double)n);
    print_arr(array, n);

    Bucket* buckets = create_buckets(n, 10);
    if (bucket_sort(array, n, buckets, n, bucket_idx) == FAILURE) {
        destroy_buckets(buckets, n);
        free(array);
        exit(EXIT_FAILURE);
    }
    printf("\n\n\n");

    print_arr(array, n);

    destroy_buckets(buckets, n);
    free(array);
    return EXIT_SUCCESS;
}
