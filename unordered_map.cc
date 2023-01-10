#include <bits/stdint-intn.h>
#include <bits/types/struct_timeval.h>
#include <cstdio>
#include <random>
#include <string>
#include <sys/time.h>
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
  printf("STL unordered_map benchmark\n");
  printf("Put %d elements, then get %d elements\n", kOpNum, kOpNum);

  vector<KvPair> kvs;
  GenerateKvPairs(kvs);
  printf("    generated key-value pairs, start testing...\n");

  // test put
  int64_t start_ts = GetUs();
  for (const auto &x : kvs) {
    hash_map[x.key] = x.value;
  }
  int64_t used_time_in_us = GetUs() - start_ts;
  printf("  put %.4f Mops, %d elements in %.4f s\n",
         kOpNum / static_cast<double>(used_time_in_us), kOpNum,
         static_cast<double>(used_time_in_us) / 1000000);

  // test get
  start_ts = GetUs();
  for (auto &x : kvs) {
    int32_t value = hash_map[x.key];
    if (value != x.value) {
      // 因为随机生成 key 的时候可能有冲突，不报错
      x.value = value;
    }
  }
  used_time_in_us = GetUs() - start_ts;
  printf("  read %.4f Mops, %d elements in %.4f s\n",
         kOpNum / static_cast<double>(used_time_in_us), kOpNum,
         static_cast<double>(used_time_in_us) / 1000000);

  // test delete
  start_ts = GetUs();
  for (const auto &x : kvs) {
    hash_map.erase(x.key);
  }
  if (!hash_map.empty()) {
    printf("ERROR delete!\n");
    exit(-1);
  }
  used_time_in_us = GetUs() - start_ts;
  printf("  delete %.4f Mops, %d elements in %.4f s\n",
         kOpNum / static_cast<double>(used_time_in_us), kOpNum,
         static_cast<double>(used_time_in_us) / 1000000);

  return 0;
}