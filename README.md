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
Put 10000000 elements, then get 10000000 elements
    generated key-value pairs, start testing...
  put 1.2783 Mops, 10000000 elements in 7.8232 s
  read 2.5116 Mops, 10000000 elements in 3.9816 s
  delete 1.6303 Mops, 10000000 elements in 6.1337 s
```