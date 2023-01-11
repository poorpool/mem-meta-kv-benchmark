# mem_meta_kv_benchmark

测试几种 KV 数据结构在内存的 get、put、delete 操作性能

1. STL unordered_map（练习）
2. 还没写 高级哈希表
3. 还没写 哈希表+trie
4. 还没写 radix tree, art

默认开启 O3 优化

## unordered_map

单线程

```
STL unordered_map benchmark, pid 3318847
16 thread(s), start_core -1
Each thread put, get, delete 10000000 elements, total 160000000 ops
[PUT] total 14.2426 Mops, in 11.2339 s, 1733.2397 MB/s, cost 19471.0036 MB
      per-thread 0.8902 Mops, 108.3275 MB/s
[GET] total 44.1036 Mops, in 3.6278 s
      per-thread 2.7565 Mops
[DEL] total 28.9419 Mops, in 5.5283 s
      per-thread 1.8089 Mops
```