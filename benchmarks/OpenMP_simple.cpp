// OpenMP program to print Hello World
// using C language
 
// OpenMP header
#include <omp.h>
 
#include <stdio.h>
#include <stdlib.h>
#include "../qsim_magic.h"
 
int main(int argc, char* argv[])
{
 
    // Beginning of parallel region
    qsim_magic_enable();
    #pragma omp parallel num_threads(2)
    {
 
        printf("Hello World... from thread = %d\n",
               omp_get_thread_num());
        
        int N = 100;
        int arr_size = 100;
        int j[arr_size];
        for (int i = 0; i<N; i++) {
            j[i%arr_size] = i;
        }
        printf("Rank %d Test size is %d\n", omp_get_thread_num(), j[N%arr_size-1]);
        
    }
    qsim_magic_disable();
    // Ending of parallel region
}