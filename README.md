# mem_meta_kv_benchmark

测试几种 KV 数据结构在内存的 get、put、delete 操作性能

1. STL unordered_map（练习）
2. 还没写 高级哈希表
3. 还没写 哈希表+trie
4. 还没写 radix tree, art

默认开启 O3 优化，绑核

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

**16 线程（默认哈希函数）**：

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