# mem_meta_kv_benchmark

测试几种 KV 数据结构在内存的 get、put、delete 操作性能

1. STL unordered_map（练习）
2. 还没写 高级哈希表
3. 还没写 哈希表+trie
4. 还没写 radix tree, art

## unordered_map

单线程

```
STL unordered_map benchmark
Put 10000000 elements sequentially, then get 10000000 elements sequentially
    generated key-value pairs, start testing...
  put 1.0742 Mops, 10000000 elements in 9.3091 s
  read 1.4089 Mops, 10000000 elements in 7.0979 s
  delete 1.1450 Mops, 10000000 elements in 8.7336 s
```