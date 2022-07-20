#!/bin/bash
echo "generating data"
g++ data_generator.cpp  -std=c++11 -O3 -fopenmp -o data_gen
./data_gen

echo "compiling knn"
g++ knn.cpp -O3 -o knn -std=c++11 -mcmodel=medium -fopenmp
# ./knn