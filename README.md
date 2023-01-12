# mem_meta_kv_benchmark

测试几种 KV 数据结构在内存的 get、put、delete 操作性能

1. STL unordered_map（练习）
2. 还没写 高级哈希表
3. 还没写 哈希表+trie
4. 还没写 radix tree, art

默认开启 O3 优化，绑核

编译运行

```bash
./build.sh && ./run.sh <thread_num> <start_core>
```

## unordered_map

（估计ops上升到25000000还会更低，怕爆内存就没测）

1 线程

```
STL unordered_map benchmark, pid 3324028
1 thread(s), start_core 40
Each thread put, get, delete 10000000 elements, total 10000000 ops
[PUT] total 1.7573 Mops, in 5.6906 s, 214.2265 MB/s, cost 1219.0843 MB
      per-thread 1.7573 Mops, 214.2265 MB/s
[GET] total 3.9607 Mops, in 2.5248 s
      per-thread 3.9607 Mops
[DEL] total 2.3387 Mops, in 4.2758 s
      per-thread 2.3387 Mops
```

16 线程

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

### ankerl::unordered_dense::{map, set}

作者在 https://martin.ankerl.com/2022/08/27/hashmap-bench-01/ 中宣称这是一个最好的哈希。这篇文章很多网站都有引用，可信度和专业度还是比较高的。作者宣称这个删除会慢一些、没有 stable reference，但是对于 IO500 的需求是足够了。

https://github.com/martinus/unordered_dense

安装：

```bash
git clone git@github.com:martinus/unordered_dense.git
cd unordered_dense

mkdir build && cd build
cmake ..
cmake --build . --target install
```

官方 example:

```cpp
#include <ankerl/unordered_dense.h>

#include <iostream>

auto main() -> int {
    auto map = ankerl::unordered_dense::map<int, std::string>();
    map[123] = "hello";
    map[987] = "world!";

    for (auto const& [key, val] : map) {
        std::cout << key << " => " << val << std::endl;
    }
}
```

单线程（默认哈希函数）：

```
ankerl::unordered_dense::map benchmark, pid 3361685
1 thread(s), start_core 40
Each thread put, get, delete 25000000 elements, total 25000000 ops
[PUT] total 3.4378 Mops, in 7.2721 s, 339.7593 MB/s, cost 2470.7564 MB
      per-thread 3.4378 Mops, 339.7593 MB/s
[GET] total 8.8703 Mops, in 2.8184 s
      per-thread 8.8703 Mops
[DEL] total 4.0132 Mops, in 6.2295 s
      per-thread 4.0132 Mops
```

4 线程，增大测试大小

```
ankerl::unordered_dense::map benchmark, pid 3364887
4 thread(s), start_core 40
Each thread put, get, delete 100000000 elements, total 400000000 ops
[PUT] total 7.5701 Mops, in 52.8398 s, 747.4531 MB/s, cost 39495.2581 MB
      per-thread 1.8925 Mops, 186.8633 MB/s
[GET] total 23.2180 Mops, in 17.2280 s
      per-thread 5.8045 Mops
[DEL] total 9.8496 Mops, in 40.6108 s
      per-thread 2.4624 Mops
```

16 线程（默认哈希函数）：

```
ankerl::unordered_dense::map benchmark, pid 3360026
16 thread(s), start_core 40
Each thread put, get, delete 25000000 elements, total 400000000 ops
[PUT] total 18.9509 Mops, in 21.1072 s, 1871.1319 MB/s, cost 39494.3447 MB
      per-thread 1.1844 Mops, 116.9457 MB/s
[GET] total 79.0079 Mops, in 5.0628 s
      per-thread 4.9380 Mops
[DEL] total 38.0489 Mops, in 10.5128 s
      per-thread 2.3781 Mops
```

16 线程 std::hash：

```
ankerl::unordered_dense::map benchmark, pid 3362193
16 thread(s), start_core 40
Each thread put, get, delete 25000000 elements, total 400000000 ops
[PUT] total 18.1298 Mops, in 22.0631 s, 1790.0691 MB/s, cost 39494.4922 MB
      per-thread 1.1331 Mops, 111.8793 MB/s
[GET] total 70.8526 Mops, in 5.6455 s
      per-thread 4.4283 Mops
[DEL] total 40.7844 Mops, in 9.8077 s
      per-thread 2.5490 Mops
```

16 线程 reserve 两倍空间：

```
ankerl::unordered_dense::map benchmark, pid 3368134
16 thread(s), start_core 40
Each thread put, get, delete 25000000 elements, total 400000000 ops
[PUT] total 29.0868 Mops, in 13.7519 s, 2559.5760 MB/s, cost 35199.1357 MB
      per-thread 1.8179 Mops, 159.9735 MB/s
[GET] total 130.5646 Mops, in 3.0636 s
      per-thread 8.1603 Mops
[DEL] total 48.9582 Mops, in 8.1702 s
      per-thread 3.0599 Mops
```

LD_PRELOAD=libjemalloc.so 单线程预留两倍空间

```
ankerl::unordered_dense::map benchmark, pid 3802564
1 thread(s), start_core 40
Each thread put, get, delete 25000000 elements, total 25000000 ops
[PUT] total 5.7464 Mops, in 4.3506 s, 419.8304 MB/s, cost 1826.4965 MB
      per-thread 5.7464 Mops, 419.8304 MB/s
[GET] total 11.6225 Mops, in 2.1510 s
      per-thread 11.6225 Mops
[DEL] total 5.1305 Mops, in 4.8728 s
      per-thread 5.1305 Mops
```

**LD_PRELOAD=libjemalloc.so 16 线程预留两倍空间**

```
ankerl::unordered_dense::map benchmark, pid 3803136
16 thread(s), start_core 40
Each thread put, get, delete 25000000 elements, total 400000000 ops
[PUT] total 64.6029 Mops, in 6.1917 s, 4720.2056 MB/s, cost 29225.9840 MB
      per-thread 4.0377 Mops, 295.0129 MB/s
[GET] total 131.5404 Mops, in 3.0409 s
      per-thread 8.2213 Mops
[DEL] total 59.9129 Mops, in 6.6764 s
      per-thread 3.7446 Mops
```

### emhash8

https://github.com/ktprime/emhash

并没有那么棒

单线程 reserve 两倍空间

```
emhash8::HashMap benchmark, pid 3397552
1 thread(s), start_core 40
Each thread put, get, delete 25000000 elements, total 25000000 ops
[PUT] total 3.9116 Mops, in 6.3912 s, 344.2353 MB/s, cost 2200.0927 MB
      per-thread 3.9116 Mops, 344.2353 MB/s
[GET] total 5.6083 Mops, in 4.4577 s
      per-thread 5.6083 Mops
[DEL] total 4.9624 Mops, in 5.0379 s
      per-thread 4.9624 Mops
```

16 线程 reserve 两倍空间

```
emhash8::HashMap benchmark, pid 3398153
16 thread(s), start_core 40
Each thread put, get, delete 25000000 elements, total 400000000 ops
[PUT] total 29.2382 Mops, in 13.6807 s, 2572.9841 MB/s, cost 35200.2908 MB
      per-thread 1.8274 Mops, 160.8115 MB/s
[GET] total 59.3415 Mops, in 6.7406 s
      per-thread 3.7088 Mops
[DEL] total 63.4724 Mops, in 6.3020 s
      per-thread 3.9670 Mops
```

### radix_tree

在key为`file.mdtest.X.Y`value为int32_t的格式下，内存占用基本上最小了。因为使用了 hugepage，VmRSS 测的不准，内存占用以代码输出为准。

因为数据结构是字典树，所以效率跟key的结构有着很大的关系

TLDR:
- 顺序key put 137 Mops，get 177 Mops，实际使用的trie节点占用的空间0.91 GB/s
- 随机key put 26 Mops，get 27 Mops，实际使用的trie节点占用的空间2.2GB/s（因为Mops小，一次操作的占用空间更大）

**顺序key**：

```cpp
sprintf(kvs[i].s, "file.mdtest.%d.%d", i / 10000, i);
```

单线程：

```
radix_tree benchmark, pid 3764370
1 thread(s), start_core 40
Each thread put, get, delete 25000000 elements, total 25000000 ops
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_0-0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Trie Use 2779140/40000000, 0.165650GB/2.384186GB
[PUT] total 11.6053 Mops, in 2.1542 s
      per-thread 11.6053 Mops
[GET] total 14.2320 Mops, in 1.7566 s
      per-thread 14.2320 Mops
[DEL] total 13.4661 Mops, in 1.8565 s
      per-thread 13.4661 Mops
```

**16 线程**：

```
radix_tree benchmark, pid 3764731
16 thread(s), start_core 40
Each thread put, get, delete 25000000 elements, total 400000000 ops
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_1-0
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_2-0
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_3-0
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_0-0
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_4-0
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_5-0
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_6-0
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_7-0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_9-0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_12-0
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_10-0
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_8-0
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_11-0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_13-0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_15-0
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_14-0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Trie Use 2779140/40000000, 0.165650GB/2.384186GB
MCatFS worker0 Info: Trie Use 2779140/40000000, 0.165650GB/2.384186GB
MCatFS worker0 Info: Trie Use 2779140/40000000, 0.165650GB/2.384186GB
MCatFS worker0 Info: Trie Use 2779140/40000000, 0.165650GB/2.384186GB
MCatFS worker0 Info: Trie Use 2779140/40000000, 0.165650GB/2.384186GB
MCatFS worker0 Info: Trie Use 2779140/40000000, 0.165650GB/2.384186GB
MCatFS worker0 Info: Trie Use 2779140/40000000, 0.165650GB/2.384186GB
MCatFS worker0 Info: Trie Use 2779140/40000000, 0.165650GB/2.384186GB
MCatFS worker0 Info: Trie Use 2779140/40000000, 0.165650GB/2.384186GB
MCatFS worker0 Info: Trie Use 2779140/40000000, 0.165650GB/2.384186GB
MCatFS worker0 Info: Trie Use 2779140/40000000, 0.165650GB/2.384186GB
MCatFS worker0 Info: Trie Use 2779140/40000000, 0.165650GB/2.384186GB
MCatFS worker0 Info: Trie Use 2779140/40000000, 0.165650GB/2.384186GB
MCatFS worker0 Info: Trie Use 2779140/40000000, 0.165650GB/2.384186GB
MCatFS worker0 Info: Trie Use 2779140/40000000, 0.165650GB/2.384186GB
MCatFS worker0 Info: Trie Use 2779140/40000000, 0.165650GB/2.384186GB
[PUT] total 137.6324 Mops, in 2.9063 s
      per-thread 8.6020 Mops
[GET] total 177.0181 Mops, in 2.2597 s
      per-thread 11.0636 Mops
[DEL] total 153.1355 Mops, in 2.6121 s
      per-thread 9.5710 Mops
```

**随机key**：

```cpp
sprintf(kvs[i].s, "file.mdtest.%d.%d", dis(gen), dis(gen));
```

单线程，NUMA Allocator 预留了 2.5 GB `NumaAllocator atr(3 * GB, huge_name);`，然后 Trie 预留了 40000000 个节点 2.38 GB，实际使用了 2.11 GB。想要哪个口径的数据，就把所有线程这些数据加起来即可：

```
radix_tree benchmark, pid 3765934
1 thread(s), start_core 40
Each thread put, get, delete 25000000 elements, total 25000000 ops
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_0-0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Trie Use 35433634/40000000, 2.112009GB/2.384186GB
[PUT] total 2.4082 Mops, in 10.3810 s
      per-thread 2.4082 Mops
[GET] total 2.3116 Mops, in 10.8150 s
      per-thread 2.3116 Mops
[DEL] total 2.1434 Mops, in 11.6637 s
      per-thread 2.1434 Mops
```

16 线程

```
radix_tree benchmark, pid 3766331
16 thread(s), start_core 40
Each thread put, get, delete 25000000 elements, total 400000000 ops
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_1-0
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_0-0
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_3-0
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_2-0
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_4-0
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_7-0
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_5-0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_6-0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_8-0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_9-0
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_10-0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_13-0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_11-0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_12-0
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_14-0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Trie Create, Max node 40000000, 2.384186GB
MCatFS worker0 Info: Huge Seg file create: /dev/hugepages/radix_cyx_test_15-0
MCatFS worker0 Info: Huge seg current_numa=req_numa=0
MCatFS worker0 Info: Trie Use 35436588/40000000, 2.112185GB/2.384186GB
MCatFS worker0 Info: Trie Use 35436660/40000000, 2.112190GB/2.384186GB
MCatFS worker0 Info: Trie Use 35438685/40000000, 2.112310GB/2.384186GB
MCatFS worker0 Info: Trie Use 35435053/40000000, 2.112094GB/2.384186GB
MCatFS worker0 Info: Trie Use 35436602/40000000, 2.112186GB/2.384186GB
MCatFS worker0 Info: Trie Use 35436210/40000000, 2.112163GB/2.384186GB
MCatFS worker0 Info: Trie Use 35435194/40000000, 2.112102GB/2.384186GB
MCatFS worker0 Info: Trie Use 35435380/40000000, 2.112113GB/2.384186GB
MCatFS worker0 Info: Trie Use 35434176/40000000, 2.112041GB/2.384186GB
MCatFS worker0 Info: Trie Use 35435880/40000000, 2.112143GB/2.384186GB
MCatFS worker0 Info: Trie Use 35436375/40000000, 2.112173GB/2.384186GB
MCatFS worker0 Info: Trie Use 35435411/40000000, 2.112115GB/2.384186GB
MCatFS worker0 Info: Trie Use 35435677/40000000, 2.112131GB/2.384186GB
MCatFS worker0 Info: Trie Use 35433953/40000000, 2.112028GB/2.384186GB
MCatFS worker0 Info: Trie Use 35434954/40000000, 2.112088GB/2.384186GB
MCatFS worker0 Info: Trie Use 35435377/40000000, 2.112113GB/2.384186GB
[PUT] total 26.1067 Mops, in 15.3218 s
      per-thread 1.6317 Mops
[GET] total 27.0473 Mops, in 14.7889 s
      per-thread 1.6905 Mops
[DEL] total 24.6983 Mops, in 16.1955 s
      per-thread 1.5436 Mops
```
