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
STL unordered_map benchmark, pid 3298532
Put 10000000 elements, then get 10000000 elements
    generated key-value pairs, start testing...
  put 1.8657 Mops, 227.0397 MB/s, 10000000 elements in 5.3599 s, cost 1216.9052 MB
  get 3.9673 Mops, 0.0000 MB/s, 10000000 elements in 2.5206 s, cost 0.0000 MB
  delete 2.8277 Mops, 0.0000 MB/s, 10000000 elements in 3.5364 s, cost 0.0000 MB
```