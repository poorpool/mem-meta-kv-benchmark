// NOLINTBEGIN
#pragma once

#include <algorithm>
#include <atomic>
#include <bits/extc++.h>
#include <condition_variable>
#include <cxxabi.h>
#include <emmintrin.h>
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <functional>
#include <iostream>
#include <malloc.h>
#include <mutex>
#include <pthread.h>
#include <queue>
#include <sched.h>
#include <set>
#include <sstream>
#include <stddef.h> //offsetof
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <thread>
#include <typeinfo>
#include <unistd.h>
#include <unordered_map>
#include <vector>

using __gnu_pbds::gp_hash_table;
using std::atomic;
using std::cerr;
using std::condition_variable;
using std::cout;
using std::endl;
using std::mutex;
using std::pair;
using std::priority_queue;
using std::queue;
using std::set;
using std::string;
using std::thread;
using std::to_string;
using std::unique_lock;
using std::unordered_map;
using std::vector;

typedef int32_t lcore_t;

#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2 * !!(condition)]))
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define UNUSED(x) ((void)(x))

#define setoffval(ptr, off, type) (*(type *)((char *)(ptr) + (int)(off)))

#define barrier() asm volatile("" : : : "memory")

#if defined(__x86_64__)
#define rmb() asm volatile("lfence" ::: "memory")
#define wmb() asm volatile("sfence" ::: "memory")
#define mb() asm volatile("mfence" ::: "memory")

#define cpu_relax() asm volatile("pause\n" : : : "memory")
#define cpu_pause() __mm_pause()

#elif defined(__arm64__) || defined(__aarch64__)

#define rmb() asm volatile("dmb ishld" : : : "memory")
#define wmb() asm volatile("dmb ishst" : : : "memory")
#define mb() asm volatile("dmb ish" : : : "memory")
#define cpu_relax() asm volatile("yield" ::: "memory")
#else
#error "unsupported arch"
#endif

#define atomic_xadd(P, V) __sync_fetch_and_add((P), (V))
#define cmpxchg(P, O, N) __sync_bool_compare_and_swap((P), (O), (N))
#define atomic_inc(P) __sync_add_and_fetch((P), 1)
#define atomic_dec(P) __sync_add_and_fetch((P), -1)
#define atomic_add(P, V) __sync_add_and_fetch((P), (V))
#define atomic_set_bit(P, V) __sync_or_and_fetch((P), 1 << (V))
#define atomic_clear_bit(P, V) __sync_and_and_fetch((P), ~(1 << (V)))
#define atomic_xor(P, V) __sync_xor_and_fetch((P), (V))

#define ALIGN_DOWN(val, align)                                                 \
  (decltype(val))((val) & (~((decltype(val))((align)-1))))
#define ALIGN_UP(val, align)                                                   \
  ALIGN_DOWN(((val) + ((decltype(val))(align)-1)), align)

#define POWEROF2(x) ((((x)-1) & (x)) == 0)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define CACHELINE_SIZE 64
#define CACHELINE_MASK (CACHELINE_SIZE - 1)
#define MCAT_CACHE_ALIGNED __attribute__((aligned(CACHELINE_SIZE)))

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#define MCAT_ALWAYS_INLINE inline __attribute__((always_inline))
#define MCAT_ALIGNED(x) __attribute__((aligned(x)))
#define MCAT_PACKED __attribute__((__packed__))

#define MCAT_PATH_MAX_LEN 256

#define KB (1UL << 10)
#define MB (KB << 10)
#define GB (MB << 10)

#define RESET "\033[0m"
#define BLACK "\033[30m"              /* Black */
#define RED "\033[31m"                /* Red */
#define GREEN "\033[32m"              /* Green */
#define YELLOW "\033[33m"             /* Yellow */
#define BLUE "\033[34m"               /* Blue */
#define MAGENTA "\033[35m"            /* Magenta */
#define CYAN "\033[36m"               /* Cyan */
#define WHITE "\033[37m"              /* White */
#define BOLDBLACK "\033[1m\033[30m"   /* Bold Black */
#define BOLDRED "\033[1m\033[31m"     /* Bold Red */
#define BOLDGREEN "\033[1m\033[32m"   /* Bold Green */
#define BOLDYELLOW "\033[1m\033[33m"  /* Bold Yellow */
#define BOLDBLUE "\033[1m\033[34m"    /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m" /* Bold Magenta */
#define BOLDCYAN "\033[1m\033[36m"    /* Bold Cyan */
#define BOLDWHITE "\033[1m\033[37m"   /* Bold White */

extern char *mcat_get_hostname();

#define PRINT_NAME "MCatFS"
#ifdef MCAT_DEBUG
#define p_debug(fmt, ...)                                                      \
  printf(PRINT_NAME " %s Debug: " BOLDCYAN fmt RESET "\n",                     \
         mcat_get_hostname(), ##__VA_ARGS__)
#else
#define p_debug(fmt, ...) ((void)0)
#endif

#define p_info(fmt, ...)                                                       \
  printf(PRINT_NAME " %s Info: " GREEN fmt RESET "\n", mcat_get_hostname(),    \
         ##__VA_ARGS__)

#define p_err(fmt, ...)                                                        \
  fprintf(stderr, PRINT_NAME " %s Error %s:%d  " RED fmt RESET "\n",           \
          mcat_get_hostname(), __FILE__, __LINE__, ##__VA_ARGS__)

static inline vector<string> split(const string &s, const char &delim = ' ') {
  vector<string> tokens;
  size_t lastPos = s.find_first_not_of(delim, 0);
  size_t pos = s.find(delim, lastPos);
  while (lastPos != string::npos) {
    tokens.emplace_back(s.substr(lastPos, pos - lastPos));
    lastPos = s.find_first_not_of(delim, pos);
    pos = s.find(delim, lastPos);
  }
  return tokens;
}

static inline void print_trace(void) {
  fflush(stdout);
  void *array[20];
  size_t size;
  char **strings;
  size_t i;
  size = backtrace(array, 20);
  strings = backtrace_symbols(array, size);
  printf("Obtained %zd stack frames.\n", size);

  for (i = 0; i < size; i++) {
    cout << "[" << i << "] ";
    cerr << strings[i] << endl;
  }
  free(strings);
}

#define p_err_assert(b, fmt, ...)                                              \
  {                                                                            \
    if (unlikely(!(b))) {                                                      \
      fflush(stdout);                                                          \
      fprintf(stderr, PRINT_NAME " %s %s:%d " BOLDRED fmt RESET "\n",          \
              mcat_get_hostname(), __FILE__, __LINE__, ##__VA_ARGS__);         \
      print_trace();                                                           \
      sync();                                                                  \
      abort();                                                                 \
    }                                                                          \
  }

#define p_assert(b) p_err_assert(b, "")

static inline int64_t get_us() {
  struct timeval t1;
  gettimeofday(&t1, NULL);
  return t1.tv_sec * 1000000L + t1.tv_usec;
}

static inline int xdigit2val(char c) {
  int32_t val;
  if (isdigit(c))
    val = c - '0';
  else if (isupper(c))
    val = c - 'A' + 10;
  else
    val = c - 'a' + 10;
  return val;
}

static inline void coremask2array(string mask, vector<int32_t> &cores) {
#define MCAT_MAX_LCORE 128
#define BITS_PER_HEX 4
  int32_t core_base = 0;
  while (!mask.empty()) {
    char c = mask.back();
    mask.pop_back();
    if (isblank(c))
      continue;
    if (c == 'x')
      break;
    if (isxdigit(c) == 0) {
      p_err_assert(0, "core mask format wrong");
    }
    int32_t val = xdigit2val(c);
    for (int i = 0; i < BITS_PER_HEX; i++) {
      if ((1 << i) & val) {
        p_err_assert((core_base + i) < MCAT_MAX_LCORE,
                     "Too many core provided");
        cores.push_back(core_base + i);
      }
    }
    core_base += BITS_PER_HEX;
  }
}

template <typename T> static inline string vec2str(vector<T> &v) {
  string s = "[";
  for (int i = 0; i < (int)v.size(); i++) {
    s += to_string(v[i]) + (i + 1 != (int)v.size() ? "," : "");
  }
  s += "]";
  return s;
}

template <typename T> static inline const char *mcat_typename() {
  const char *n = typeid(T).name();
  const char *c = abi::__cxa_demangle(n, NULL, NULL, NULL);
  if (c)
    return c;
  else
    return n;
}

static inline uint64_t mcat_str_to_size(const char *str) {
  char *endptr;
  unsigned long long size;

  while (isspace((int)*str))
    str++;
  if (*str == '-')
    return 0;

  errno = 0;
  size = strtoull(str, &endptr, 0);
  if (errno)
    return 0;

  if (*endptr == ' ')
    endptr++; /* allow 1 space gap */

  switch (*endptr) {
  case 'G':
  case 'g':
    size *= 1024; /* fall-through */
  case 'M':
  case 'm':
    size *= 1024; /* fall-through */
  case 'K':
  case 'k':
    size *= 1024; /* fall-through */
  default:
    break;
  }
  return size;
}

static inline int32_t vma2numa(void *va) {
  int32_t socket_id = -1;
  char *end, *nodestr;
  uint64_t virt_addr;
  char buf[BUFSIZ];
  FILE *f;

  f = fopen("/proc/self/numa_maps", "r");
  p_assert(f);

  /* parse numa map */
  while (fgets(buf, sizeof(buf), f) != NULL) {
    /* get zone addr */
    virt_addr = strtoull(buf, &end, 16);
    if (virt_addr == 0 || end == buf) {
      p_assert(0);
    }
    if (virt_addr != (uint64_t)va)
      continue;

    /* get node id (socket id) */
    nodestr = strstr(buf, " N");
    p_assert(nodestr);
    nodestr += 2;
    end = strstr(nodestr, "=");
    p_assert(end);
    end[0] = '\0';
    end = NULL;

    socket_id = strtoul(nodestr, &end, 0);
    if ((nodestr[0] == '\0') || (end == NULL) || (*end != '\0')) {
      p_assert(0);
    }
  }
  fclose(f);
  return socket_id;
}

#define MCAT_LONG_MAX_LEN 20
static inline void mcat_l2x(uint64_t v, char *s) {
  if (unlikely(!v)) {
    strcpy(s, "0");
    return;
  }
  int32_t n = ALIGN_UP(64 - __builtin_clzll(v), 4) / 4;
  char c;
  s[n--] = '\0';
  for (; n >= 0; n--) {
    c = v & 15;
    if (likely(c < 10))
      s[n] = '0' + c;
    else
      s[n] = 'A' + c - 10;
    v >>= 4;
  }
}
// NOLINTEND