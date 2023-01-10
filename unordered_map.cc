#include "utils.h"
#include <bits/stdint-intn.h>
#include <bits/types/struct_timeval.h>
#include <cstdio>
#include <random>
#include <string>
#include <sys/time.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>
using std::mt19937;
using std::random_device;
using std::string;
using std::uniform_int_distribution;
using std::unordered_map;
using std::vector;

constexpr int kOpNum = 10000000;
unordered_map<string, int32_t> hash_map;

struct KvPair {
  string key;
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
  char key_buffer[105];

  for (int i = 0; i < kOpNum; i++) {
    sprintf(key_buffer, "file.mdtest.%d.%d", dis(gen), dis(gen));
    string key = key_buffer;
    int32_t value = dis(gen);

    kvs.push_back({key, value});
  }
}

int main() {
  int pid = getpid();
  printf("STL unordered_map benchmark, pid %d\n", pid);
  printf("Put %d elements, then get %d elements\n", kOpNum, kOpNum);

  vector<KvPair> kvs;
  GenerateKvPairs(kvs);
  printf("    generated key-value pairs, start testing...\n");

  int64_t start_ts;        // 每段测试起始时间戳，微秒
  int64_t start_b;         // 每段测试起始 VmRSS，Byte
  int64_t used_time_in_us; // 每段测试使用的微秒
  int64_t diff_b;          // 每段测试使用的 VmRSS，Byte

  // test put
  start_ts = GetUs();
  start_b = GetVmRssInB(pid);
  for (const auto &x : kvs) {
    hash_map[x.key] = x.value;
  }
  used_time_in_us = GetUs() - start_ts;
  diff_b = GetVmRssInB(pid) - start_b;
  printf("  put %.4f Mops, %.4f MB/s, %d elements in %.4f s, cost %.4f MB\n",
         kOpNum / static_cast<double>(used_time_in_us),
         static_cast<double>(diff_b) / used_time_in_us, // B/us == MB/s
         kOpNum, static_cast<double>(used_time_in_us) / 1000000,
         static_cast<double>(diff_b) / 1000000);

  // test get
  start_ts = GetUs();
  start_b = GetVmRssInB(pid);
  for (auto &x : kvs) {
    int32_t value = hash_map[x.key];
    if (value != x.value) {
      // 因为随机生成 key 的时候可能有冲突，不报错
      x.value = value;
    }
  }
  used_time_in_us = GetUs() - start_ts;
  diff_b = GetVmRssInB(pid) - start_b;
  printf("  get %.4f Mops, %.4f MB/s, %d elements in %.4f s, cost %.4f MB\n",
         kOpNum / static_cast<double>(used_time_in_us),
         static_cast<double>(diff_b) / used_time_in_us, // B/us == MB/s
         kOpNum, static_cast<double>(used_time_in_us) / 1000000,
         static_cast<double>(diff_b) / 1000000);

  // test delete
  start_ts = GetUs();
  start_b = GetVmRssInB(pid);
  for (const auto &x : kvs) {
    hash_map.erase(x.key);
  }
  if (!hash_map.empty()) {
    printf("ERROR delete!\n");
    exit(-1);
  }
  used_time_in_us = GetUs() - start_ts;
  diff_b = GetVmRssInB(pid) - start_b;
  printf("  delete %.4f Mops, %.4f MB/s, %d elements in %.4f s, cost %.4f MB\n",
         kOpNum / static_cast<double>(used_time_in_us),
         static_cast<double>(diff_b) / used_time_in_us, // B/us == MB/s
         kOpNum, static_cast<double>(used_time_in_us) / 1000000,
         static_cast<double>(diff_b) / 1000000);

  return 0;
}