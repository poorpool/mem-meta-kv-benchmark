# mem_meta_kv_benchmark

测试几种 KV 数据结构在内存的 get、put、delete 操作性能

1. STL unordered_map（练习）
2. 还没写 高级哈希表
3. 还没写 哈希表+trie
4. 还没写 radix tree, art

默认开启 O3 优化，绑核

## unordered_map

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

单线程：

```
ankerl::unordered_dense::map benchmark, pid 3325576
1 thread(s), start_core 40
Each thread put, get, delete 10000000 elements, total 10000000 ops
[PUT] total 3.5006 Mops, in 2.8567 s, 355.0110 MB/s, cost 1014.1532 MB
      per-thread 3.5006 Mops, 355.0110 MB/s
[GET] total 10.7456 Mops, in 0.9306 s
      per-thread 10.7456 Mops
[DEL] total 4.4586 Mops, in 2.2428 s
      per-thread 4.4586 Mops
```

16 线程

```
ankerl::unordered_dense::map benchmark, pid 3325677
16 thread(s), start_core 40
Each thread put, get, delete 10000000 elements, total 160000000 ops
[PUT] total 25.8268 Mops, in 6.1951 s, 2619.2087 MB/s, cost 16226.2753 MB
      per-thread 1.6142 Mops, 163.7005 MB/s
[GET] total 136.4213 Mops, in 1.1728 s
      per-thread 8.5263 Mops
[DEL] total 58.1234 Mops, in 2.7528 s
      per-thread 3.6327 Mops
```