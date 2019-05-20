#!/bin/bash


gcc -g -O0 -std=gnu99 -lcurl -pthread server_sim_bench.c threadpool.c PMurHash.c -o server_sim_bench
./server_sim_bench links/164.txt links/328.txt
cd webcache; rm -rf *; cd ..
python plot_proxy_perf.py
python3 asyncio_bench.py
