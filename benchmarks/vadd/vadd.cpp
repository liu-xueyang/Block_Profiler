// OpenMP program to run vector add
// using C language
 
// OpenMP header
#include <omp.h>
 
#include <stdio.h>
#include <stdlib.h>

#include "../../qsim_magic.h"
 
int main(int argc, char* argv[])
{
    int numElements = 1024*1024*32;
    size_t size = numElements * sizeof(float);
    int numThreads = 8; 
    int ops_per_thread = numElements / numThreads + 1;
    printf("[Vector addition of %d elements with %d threads]\n", numElements, numThreads);

    // Allocate the input vector A
    float *h_A = (float *)malloc(size);

    // Allocate the input vector B
    float *h_B = (float *)malloc(size);

    // Allocate the output vector C
    float *h_C = (float *)malloc(size);

    // Verify that allocations succeeded
    if (h_A == NULL || h_B == NULL || h_C == NULL) {
        fprintf(stderr, "Failed to allocate host vectors!\n");
        exit(EXIT_FAILURE);
    }

    // Initialize the host input vectors
    for (int i = 0; i < numElements; ++i) {
        h_A[i] = rand() / (float)RAND_MAX;
        h_B[i] = rand() / (float)RAND_MAX;
    }

    // Execute vector add in parallel
    ANNOTATE_SITE_BEGIN_WKR("vadd", 0);
    #pragma omp parallel num_threads(numThreads)
    {
 
        // printf("Hello World... from thread = %d\n",
        //        omp_get_thread_num());
        int threadId = omp_get_thread_num();
        for (int i = 0; i < ops_per_thread; i++) {
            int arr_i = threadId * ops_per_thread + i;

            if (arr_i < numElements) {
                h_C[arr_i] = h_A[arr_i] + h_B[arr_i] + 0.0f;
            }
        }
        #pragma omp barrier
    }
    ANNOTATE_SITE_END_WKR(0);
    // Ending of parallel region
}