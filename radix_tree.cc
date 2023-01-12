#include "3rdparty/mcatfs/radix_tree.h"
#include "3rdparty/mcatfs/numa_tools.h"
#include "utils.h"
#include <ankerl/unordered_dense.h>
#include <atomic>
#include <bits/stdint-intn.h>
#include <bits/types/struct_timeval.h>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <pthread.h>
#include <random>
#include <string>
#include <sys/time.h>
#include <thread>
#include <unistd.h>
#include <vector>
using std::mt19937;
using std::random_device;
using std::string;
using std::thread;
using std::uniform_int_distribution;
using std::vector;

constexpr int kOpNum = 25000000;
int thread_num; // 执行测试的线程数量
int start_core; // 这些线程从哪里开始绑核，-1 代表不绑核

struct KvPair {
  char s[64];
  int len;
  int32_t value;
};

int64_t GetUs() {
  timeval tv;
  gettimeofday(&tv, nullptr);
  return tv.tv_usec + tv.tv_sec * 1000000L;
}

// 注意：生成的 Key 有重复
void GenerateKvPairs(vector<KvPair> &kvs) {
  random_device rd;
  mt19937 gen(rd());
  uniform_int_distribution<int> dis(0, kOpNum);

  kvs = vector<KvPair>(kOpNum, KvPair{});
  for (int i = 0; i < kOpNum; i++) {
    sprintf(kvs[i].s, "file.mdtest.%d.%d", dis(gen), dis(gen));
    // sprintf(kvs[i].s, "file.mdtest.%d.%d", i / 10000, i);
    kvs[i].len = strlen(kvs[i].s);
    kvs[i].value = dis(gen);
  }
}

void PrintUsage(char *name) {
  printf("Usage:\n"
         "  single thread: %s\n"
         "  multiple threads: %s <num_of_threads> <start_core>\n",
         name, name);
}

pthread_barrier_t barrier1, barrier2, barrier3;

// idx 表示在 thread_num 中当前线程排第几
void ThreadFunc(int idx) {
  if (start_core != -1) {
    cpu_set_t cpuset;

    CPU_ZERO(&cpuset); // 初始化CPU集合，将 cpuset 置为空
    CPU_SET(idx + start_core, &cpuset); // 将本进程绑定到 CPU 上

    // 设置线程的 CPU 亲和性
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset) != 0) {
      printf("Set CPU affinity failed\n");
      exit(-1);
    }
  }
  char huge_name[35];
  sprintf(huge_name, "radix_cyx_test_%d", idx);
  NumaAllocator atr(2.5 * GB, huge_name); // 不同线程的 huge_name 必须不同
  Trie *trie = trie_create(40000000, atr, 0);
  TrieNode *root = trie_alloc_root(trie);
  vector<KvPair> kvs;
  GenerateKvPairs(kvs);

  // test put
  pthread_barrier_wait(&barrier1);
  // 主线程计时中
  pthread_barrier_wait(&barrier2);
  for (const auto &x : kvs) {
    trie_insert(trie, root, x.s, x.len, x.value);
  }
  pthread_barrier_wait(&barrier3);
  trie_print_use(trie);

  // test get
  pthread_barrier_wait(&barrier1);
  // 主线程计时中
  pthread_barrier_wait(&barrier2);
  for (auto &x : kvs) {
    int32_t value = trie_get(trie, root, x.s, x.len);
    if (value != x.value) {
      // 因为随机生成 key 的时候可能有冲突，不报错
      x.value = value;
    }
  }
  pthread_barrier_wait(&barrier3);

  // test delete
  pthread_barrier_wait(&barrier1);
  // 主线程计时中
  pthread_barrier_wait(&barrier2);
  for (const auto &x : kvs) {
    trie_del(trie, root, x.s, x.len);
  }
  pthread_barrier_wait(&barrier3);
  if (trie->use_cnt != 1) {
    printf("ERROR delete!\n");
    trie_dump(trie, root);
    trie_print_use(trie);
  }
  trie_free(trie);
}

int main(int argc, char *argv[]) {
  if (argc != 1 && argc != 3) {
    PrintUsage(argv[0]);
    return 0;
  }

  int pid = getpid();
  printf("radix_tree benchmark, pid %d\n", pid);
  if (argc == 1) {
    thread_num = 1;
    start_core = -1;
  } else {
    thread_num = atoi(argv[1]);
    start_core = atoi(argv[2]);
  }
  printf("%d thread(s), start_core %d\n", thread_num, start_core);
  printf("Each thread put, get, delete %d elements, total %ld ops\n", kOpNum,
         static_cast<int64_t>(kOpNum) * thread_num);

  vector<thread> threads;
  threads.reserve(thread_num);
  for (int i = 0; i < thread_num; i++) {
    threads.emplace_back(ThreadFunc, i);
  }

  // PUT
  pthread_barrier_init(&barrier1, nullptr, thread_num + 1);
  pthread_barrier_init(&barrier2, nullptr, thread_num + 1);
  pthread_barrier_init(&barrier3, nullptr, thread_num + 1);
  // 计时前同步
  pthread_barrier_wait(&barrier1);
  pthread_barrier_destroy(&barrier1);
  pthread_barrier_init(&barrier1, nullptr, thread_num + 1); // 为 GET 做准备

  // PUT 前同步并开始计时
  int64_t start_ts = GetUs();
  int64_t start_b = GetVmRssInB(pid);
  pthread_barrier_wait(&barrier2);
  pthread_barrier_destroy(&barrier2);

  // PUT 中……

  // PUT 后计时结束
  pthread_barrier_wait(&barrier3);
  pthread_barrier_destroy(&barrier3);
  int64_t used_time_in_us = GetUs() - start_ts;
  int64_t diff_b = GetVmRssInB(pid) - start_b;

  printf("[PUT] total %.4f Mops, in %.4f s\n"
         "      per-thread %.4f Mops\n",
         static_cast<double>(kOpNum) * thread_num / used_time_in_us,
         static_cast<double>(used_time_in_us) / 1000000,
         static_cast<double>(kOpNum) / used_time_in_us);

  // GET
  // barrier1 已经初始化
  pthread_barrier_init(&barrier2, nullptr, thread_num + 1);
  pthread_barrier_init(&barrier3, nullptr, thread_num + 1);
  // 计时前同步
  pthread_barrier_wait(&barrier1);
  pthread_barrier_destroy(&barrier1);
  pthread_barrier_init(&barrier1, nullptr, thread_num + 1); // 为 DELETE 做准备

  // GET 前同步并开始计时
  start_ts = GetUs();
  pthread_barrier_wait(&barrier2);
  pthread_barrier_destroy(&barrier2);

  // GET 中……

  // GET 后计时结束
  pthread_barrier_wait(&barrier3);
  pthread_barrier_destroy(&barrier3);
  used_time_in_us = GetUs() - start_ts;

  printf("[GET] total %.4f Mops, in %.4f s\n"
         "      per-thread %.4f Mops\n",
         static_cast<double>(kOpNum) * thread_num / used_time_in_us,
         static_cast<double>(used_time_in_us) / 1000000,
         static_cast<double>(kOpNum) / used_time_in_us);

  // DELETE
  // barrier1 已经初始化
  pthread_barrier_init(&barrier2, nullptr, thread_num + 1);
  pthread_barrier_init(&barrier3, nullptr, thread_num + 1);
  // 计时前同步
  pthread_barrier_wait(&barrier1);
  pthread_barrier_destroy(&barrier1);

  // DELETE 前同步并开始计时
  start_ts = GetUs();
  pthread_barrier_wait(&barrier2);
  pthread_barrier_destroy(&barrier2);

  // DELETE 中……

  // DELETE 后计时结束
  pthread_barrier_wait(&barrier3);
  pthread_barrier_destroy(&barrier3);
  used_time_in_us = GetUs() - start_ts;

  printf("[DEL] total %.4f Mops, in %.4f s\n"
         "      per-thread %.4f Mops\n",
         static_cast<double>(kOpNum) * thread_num / used_time_in_us,
         static_cast<double>(used_time_in_us) / 1000000,
         static_cast<double>(kOpNum) / used_time_in_us);

  for (auto &t : threads) {
    t.join();
  }

  return 0;
}