#!/bin/bash

# ./build/umap_bmtest $1 $2
# ./build/ankerl_hashmap_bmtest $1 $2
# ./build/emhash8_bmtest $1 $2
# sudo ./build/radix_tree_bmtest $1 $2
LD_PRELOAD=libjemalloc.so ./build/ankerl_hashmap_bmtest $1 $2