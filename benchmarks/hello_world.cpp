// Hello World program to test out qsim magic annotation
 
// OpenMP header
#include <omp.h>
 
#include <stdio.h>
#include <stdlib.h>
#include "../qsim_magic.h"
 
int main(int argc, char* argv[])
{
 
    // Beginning of parallel region
    qsim_magic_enable();
    int N = 100;
    int arr_size = 100;
    int j[arr_size], k[arr_size];
    for (int i = 0; i<N; i++) {
        j[i%arr_size] = i;
        k[i%arr_size] = 2*i;
    }
    printf("Finished region 1, k[50]=%d\n",  k[50]);


    qsim_magic_enable();    
    int sum[arr_size];
    for (int i = 0; i< arr_size; i++) {
        sum[i] = j[i] + k[i];
    }  
    printf("Finished region 2, sum[50]=%d\n",  sum[50]);

    qsim_magic_enable();
    int diff[arr_size];
    for (int i = 0; i< arr_size; i++) {
        diff[i] = j[i] - k[i];
    }
    printf("Finished region 3, diff[50]=%d\n",  diff[50]);

    // Ending of parallel region
}