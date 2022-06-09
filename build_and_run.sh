# Usage ./build_and_run.sh [Benchmark_location] [Trace_location]
benchmark=$1
TraceName=$2
make obj-intel64/BlockProfiler.so
pin -t obj-intel64/BlockProfiler.so -t $TraceName -- $benchmark
cd CacheSim
python3 cachesimulator.py --trace ../hello2.trace.0
cd ..
python3 TraceAnalysis/traceAnalyzer.py --trace hello2.trace.0
python3 TraceAnalysis/traceAnalyzer.py --trace hello2.trace.0.L2