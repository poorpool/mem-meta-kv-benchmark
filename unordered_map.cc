#include <bits/stdint-intn.h>
#include <bits/types/struct_timeval.h>
#include <cstdio>
#include <string>
#include <sys/time.h>
#include <unordered_map>
#include <vector>
using std::string;
using std::unordered_map;
using std::vector;

constexpr int kOpNum = 10000000;
unordered_map<string, string> hash_map;

struct KvPair {
  string key;
  string value;
};

int64_t GetUs() {
  timeval tv;
  gettimeofday(&tv, nullptr);
  return tv.tv_usec + tv.tv_sec * 1000000L;
}

void GenerateKvPairs(vector<KvPair> &kvs) {
  char key_buffer[105];
  char value_buffer[105];
  for (int i = 0; i < kOpNum; i++) {
    sprintf(key_buffer, "file.mdtest.%d.%d", i / 100000, i % 100000);
    sprintf(value_buffer, "fdferh%dtah54-", i);
    string key = key_buffer;
    string value = value_buffer + key;

    kvs.push_back({key, value});
  }
}

int main() {
  printf("STL unordered_map benchmark\n");
  printf("Put %d elements sequentially, then get %d elements sequentially\n",
         kOpNum, kOpNum);

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
  for (const auto &x : kvs) {
    string value = hash_map[x.key];
    if (value != x.value) {
      printf("ERR!\n");
      exit(-1);
    }
  }
  used_time_in_us = GetUs() - start_ts;
  printf("  read %.4f Mops, %d elements in %.4f s\n",
         kOpNum / static_cast<double>(used_time_in_us), kOpNum,
         static_cast<double>(used_time_in_us) / 1000000);

  return 0;
}