#!/bin/bash

gcc -o PoW_max_zero_prefix_in_one_minute_parallel PoW_max_zero_prefix_in_one_minute_parallel.c sha256/sha256.c sha256/sha256.h -O2 -fopenmp -lm -msse4 -msha
