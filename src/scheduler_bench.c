#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <unistd.h>
#include <sys/random.h>

#define N 10000000

typedef struct {
    double static_calc;
    double static_1;
    double dynamic_calc;
    double dynamic_1;
} RandTimes;

void write_csv(const RandTimes* times, const char* csv_path, size_t n_elems, int n_threads) {
    if (csv_path == NULL) return;

    int write_header = (access(csv_path, F_OK) != 0);

    FILE* f = fopen(csv_path, "a");
    if (f == NULL) {
        fprintf(stderr, "Failed to open CSV file: %s\n", csv_path);
        return;
    }

    if (write_header)
        fprintf(f, "n_elems,n_threads,static_calc,static_1,dynamic_calc,dynamic_1\n");

    fprintf(f, "%zu,%d,%.6f,%.6f,%.6f,%.6f\n",
            n_elems,
            n_threads,
            times->static_calc,
            times->static_1,
            times->dynamic_calc,
            times->dynamic_1);

    fclose(f);
}

// Static scheduling with chunk_size = n_elems / threads
void random_array_1(double* arr, size_t n_elems, double max) {
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

// Static scheduling with chunk_size = 1
void random_array_2(double* arr, size_t n_elems, double max) {
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

// Dynamic scheduling with chunk_size = n_elems / threads
void random_array_3(double* arr, size_t n_elems, double max) {
    size_t threads = (size_t)omp_get_max_threads();
    const size_t chunk_size = n_elems / threads;
    #pragma omp parallel
    {
        unsigned short xsubi[3];
        getentropy(xsubi, sizeof(xsubi));

        #pragma omp for schedule(dynamic, chunk_size)
        for (size_t i = 0; i < n_elems; i++) {
            arr[i] = erand48(xsubi) * max;
        }
    }
}

// Dynamic scheduling with chunk_size = 1
void random_array_4(double* arr, size_t n_elems, double max) {
    #pragma omp parallel
    {
        unsigned short xsubi[3];
        getentropy(xsubi, sizeof(xsubi));

        #pragma omp for schedule(dynamic)
        for (size_t i = 0; i < n_elems; i++) {
            arr[i] = erand48(xsubi) * max;
        }
    }
}

int main(int argc, char* argv[]) {
    size_t n = N;
    if (argc >= 2) {
        n = (size_t)strtoul(argv[1], NULL, 10);
    }

    const char* csv_path = (argc >= 3) ? argv[2] : NULL;

    double* array = (double*)malloc(n * sizeof(double));
    if (array == NULL) {
        fprintf(stderr, "Failed to allocate array of size %zu\n", n);
        exit(EXIT_FAILURE);
    }

    RandTimes times;

    double start, end;

    start = omp_get_wtime();
    random_array_1(array, n, n);
    end = omp_get_wtime();
    times.static_calc = end - start;

    start = omp_get_wtime();
    random_array_2(array, n, n);
    end = omp_get_wtime();
    times.static_1 = end - start;

    start = omp_get_wtime();
    random_array_3(array, n, n);
    end = omp_get_wtime();
    times.dynamic_calc = end - start;

    start = omp_get_wtime();
    random_array_4(array, n, n);
    end = omp_get_wtime();
    times.dynamic_1 = end - start;

    write_csv(&times, csv_path, n, omp_get_max_threads());

    return EXIT_SUCCESS;
}
