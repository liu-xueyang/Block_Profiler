// Hello World program to test out qsim magic annotation
 
// OpenMP header
#include <omp.h>
 
#include <stdio.h>
#include <stdlib.h>
#include "../qsim_magic.h"
 
int main(int argc, char* argv[])
{
 
    // Beginning of parallel region
    int N = 200;
    int arr_size = 2000;
    printf("Testing hello world with array size %d\n", arr_size);
    
    ANNOTATE_SITE_BEGIN_WKR("R1", 0);
    int j[arr_size], k[arr_size];
    for (int i = 0; i<N; i++) {
        j[i%arr_size] = i;
        k[i%arr_size] = 2*i;
    }
    ANNOTATE_SITE_END_WKR(0);


    ANNOTATE_SITE_BEGIN_WKR("R2", 0);
    int sum[arr_size];
    for (int i = 0; i< arr_size; i++) {
        sum[i] = j[i] + k[i];
    }
    ANNOTATE_SITE_END_WKR(0);
    // printf("Finished region 2, sum[50]=%d\n",  sum[50]);

    ANNOTATE_SITE_BEGIN_WKR("R3", 0);
    int diff[arr_size];
    for (int i = 0; i< arr_size; i++) {
        diff[i] = j[i] - k[i];
    }
    ANNOTATE_SITE_END_WKR(0);
    // printf("Finished region 3, diff[50]=%d\n",  diff[50]);

    // Ending of parallel region
}