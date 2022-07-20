# Usage ./build_and_run.sh [Benchmark_location] [Trace_location]
# Example ./build_and_run.sh benchmarks/EISC/knn/knn benchmarks/EISC/knn/trace
benchmark=$1
ProfileName=$2
rm obj-intel64/*
make obj-intel64/BlockProfiler.so
pin -t obj-intel64/BlockProfiler.so -t $ProfileName -- $benchmark
# cd CacheSim
# python3 cachesimulator.py --trace ../$TraceName.0
# cd ..
# python3 TraceAnalysis/traceAnalyzer.py --trace $TraceName.0
# python3 TraceAnalysis/traceAnalyzer.py --trace $TraceName.0.L2