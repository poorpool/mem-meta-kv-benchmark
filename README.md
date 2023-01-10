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
STL unordered_map benchmark, pid 3297947
Put 10000000 elements, then get 10000000 elements
    generated key-value pairs, start testing...
  put 1.9019 Mops, 1216.9667 MB/s, 10000000 elements in 5.2580 s
  read 3.9722 Mops, 0.0000 MB/s, 10000000 elements in 2.5175 s
  delete 2.8667 Mops, 0.0000 MB/s, 10000000 elements in 3.4883 s
```