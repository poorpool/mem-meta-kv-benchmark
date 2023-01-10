# mem_meta_kv_benchmark

测试几种 KV 数据结构在内存的 get、put、delete 操作性能

1. STL unordered_map（练习）
2. 还没写 高级哈希表
3. 还没写 哈希表+trie
4. 还没写 radix tree, art

## unordered_map

单线程

```
STL unordered_map benchmark, pid 3297086
Put 10000000 elements, then get 10000000 elements
    generated key-value pairs, start testing...
  put 1.2953 Mops, 1216.9052 MB/s, 10000000 elements in 7.7202 s
  read 2.5864 Mops, 0.0000 MB/s, 10000000 elements in 3.8663 s
  delete 1.5981 Mops, 0.0000 MB/s, 10000000 elements in 6.2575 s
```