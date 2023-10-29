#!/bin/bash

gcc -o PoW_max_zero_prefix_in_one_minute_parallel PoW_max_zero_prefix_in_one_minute_parallel.c -lssl -lcrypto -O1 -fopenmp -lm