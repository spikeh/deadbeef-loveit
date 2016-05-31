#!/bin/sh
gcc -Wall -fPIC -std=c99 -shared -O2 -o loveit.so loveit.c && echo "Built loveit.so"
