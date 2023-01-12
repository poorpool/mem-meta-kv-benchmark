// NOLINTBEGIN
#pragma once

#include "common.h"
#include "huge.h"
#include <numa.h>

static inline vector<int32_t> get_task_numa_list() {
  vector<int32_t> rc;
  struct bitmask *node_mask = numa_get_run_node_mask();
  for (int32_t i = 0; i < (int)node_mask->size; i++) {
    if (numa_bitmask_isbitset(node_mask, i))
      rc.push_back(i);
  }
  numa_bitmask_free(node_mask);
  return rc;
}

// bad proformance , just for init memalloc
class NumaAllocator {
public:
  const static int64_t default_heap_size = 4 * MB;

  const static int64_t ele_size = 64;

private:
  int64_t heap_size;
  HugeCtx huge_ctx;
  enum { FLAG_FREE = 9, FLAG_ALLOCED };
  struct FLHead {
    int32_t alloc_flag;
    int32_t numa_id;
    int64_t ele_num;
  } MCAT_ALIGNED(ele_size);
  static_assert(sizeof(FLHead) == ele_size &&
                (default_heap_size % ele_size) == 0);

  int32_t max_node_id;
  mutex lk;
  unordered_map<int32_t, set<FLHead *>> numa_alloc_map;

  void *alloc_mem(int64_t size, int32_t numa_id) {
    if (huge_ctx.is_inited() == false) {
      // normal memory
      return numa_alloc_onnode(size, numa_id);
    } else {
      return huge_ctx.allocSeg(size, numa_id);
    }
  }
  FLHead *FL_add_new_numa(int32_t numa_id) {
    FLHead *head = (FLHead *)alloc_mem(heap_size, numa_id);
    p_assert(head);
    head->alloc_flag = FLAG_FREE;
    head->numa_id = numa_id;
    head->ele_num = (heap_size - sizeof(FLHead)) / ele_size;
    return head;
  }

  static void FL_free(set<FLHead *> &st, FLHead *f) {
    f->alloc_flag = FLAG_FREE;
    char *edg = (char *)f + sizeof(FLHead) + f->ele_num * ele_size;
    // merge back
    {
      auto it = st.lower_bound(f);
      if (it != st.end()) {
        FLHead *b = *it;
        p_assert((char *)b >= edg);
        p_assert(b->alloc_flag == FLAG_FREE);
        if ((char *)b == edg) {
          // do merge
          st.erase(b);
          f->ele_num += b->ele_num + sizeof(FLHead) / ele_size;
        }
      }
    }
    // merge front
    {
      auto it = st.lower_bound(f);
      if (it != st.begin()) {
        FLHead *p = *(--it);
        p_assert(p->alloc_flag == FLAG_FREE);
        char *p_edg = (char *)p + sizeof(FLHead) + p->ele_num * ele_size;
        p_assert((char *)f >= p_edg);
        if ((char *)f == p_edg) {
          // do merge
          st.erase(p);
          p->ele_num += f->ele_num + sizeof(FLHead) / ele_size;
          f = p;
        }
      }
    }
    // insert f
    st.insert(f);
  }

  static void *FL_alloc(set<FLHead *> &st, int64_t size) {
    size = ALIGN_UP(size, ele_size);
    FLHead *h = NULL;
    for (auto &i : st) {
      if (i->ele_num * ele_size >= size) {
        h = i;
        break;
      }
    }
    if (!h)
      return NULL;
    if (size + (int64_t)sizeof(FLHead) < h->ele_num * ele_size) {
      // split
      FLHead *node = (FLHead *)((char *)h + sizeof(FLHead) + size);
      node->alloc_flag = FLAG_FREE;
      node->ele_num =
          (h->ele_num * ele_size - size - sizeof(FLHead)) / ele_size;
      node->numa_id = h->numa_id;
      h->ele_num = size / ele_size;
      st.insert(node);
    }
    st.erase(h);
    h->alloc_flag = FLAG_ALLOCED;
    return (char *)h + sizeof(FLHead);
  }

public:
  NumaAllocator(int64_t size = default_heap_size) : heap_size(size) {
    p_assert(POWEROF2(ele_size));
    max_node_id = numa_max_node();
  }
  NumaAllocator(int64_t size, string huge_name) : NumaAllocator(size) {
    huge_ctx.init(huge_name);
  }

  void *alloc(int64_t size, int32_t numa_id, bool safe = true) {
    p_assert(numa_id <= max_node_id);
    unique_lock<mutex> lg(lk);
    void *buf = NULL;
    auto it = numa_alloc_map.find(numa_id);
    if (it == numa_alloc_map.end()) {
      // create new numa allocator
      numa_alloc_map[numa_id].insert(FL_add_new_numa(numa_id));
    }

    buf = FL_alloc(numa_alloc_map[numa_id], size);
    if (safe)
      p_assert(buf);
    return buf;
  }
  void free(void *buf) {
    FLHead *h = (FLHead *)((char *)buf - sizeof(FLHead));
    p_assert(h->alloc_flag == FLAG_ALLOCED);
    unique_lock<mutex> lg(lk);
    auto it = numa_alloc_map.find(h->numa_id);
    p_assert(it != numa_alloc_map.end());
    FL_free(it->second, h);
  }
  int32_t dump() {
    p_debug("Max Numa ID %d", max_node_id);
    unique_lock<mutex> lg(lk);
    p_info("Numa Allocator Dump :");
    for (auto &i : numa_alloc_map) {
      p_info("\tNuma %d", i.first);
      for (auto &j : i.second) {
        p_assert(j->alloc_flag == FLAG_FREE);
        p_info("\t\t%p size %ld", j, j->ele_num * ele_size);
      }
    }
    p_info("Dump done");
    return (int32_t)numa_alloc_map.size();
  }
};

class ThreadNumaTable {
  static thread_local int32_t numa_id;
  static NumaAllocator *numa_allocator;

public:
  static void set_numa_id(int32_t id) { numa_id = id; }
  // default value -1
  static int32_t get_numa_id() { return numa_id; }
  static void init_allocator(int64_t size, const string &name) {
    p_assert(numa_allocator == NULL);
    numa_allocator = new NumaAllocator(size, name);
  }
  static inline NumaAllocator &get_global_allocator() {
    p_assert(numa_allocator);
    return *numa_allocator;
  }
};

static inline void *mcat_malloc(int64_t size, int32_t numa_id) {
  return ThreadNumaTable::get_global_allocator().alloc(size, numa_id);
}

static inline void mcat_free(void *p) {
  ThreadNumaTable::get_global_allocator().free(p);
}

template <class T> class CxxNumaAllocator {
public:
  using value_type = T;
  using pointer = T *;
  using const_pointer = const T *;

  using void_pointer = void *;
  using const_void_pointer = const void *;

  using size_type = size_t;
  using difference = std::ptrdiff_t;

  template <class U> struct rebind {
    using other = CxxNumaAllocator<U>;
  };

  CxxNumaAllocator(){};
  ~CxxNumaAllocator() = default;

  // 分配内存
  pointer allocate(size_type num) {
    int32_t numa_id = ThreadNumaTable::get_numa_id();
    p_err_assert(numa_id != -1, "Haven't set numa id");
    // p_debug("alloc %s * %ld on numa %d", mcat_typename<T>(), num, numa_id);
    return static_cast<pointer>(ThreadNumaTable::get_global_allocator().alloc(
        sizeof(value_type) * num, 0));
  }

  // 分配内存
  pointer allocate(size_type num, const_void_pointer hint) {
    return allocate(num);
  }

  // 释放内存
  void deallocate(pointer p, size_type num) {
    // p_debug("free %s * %ld", mcat_typename<T>(), num);
    ThreadNumaTable::get_global_allocator().free(p);
  }

  // 分配器支持最大的内存数
  size_type max_size() const { return std::numeric_limits<size_type>::max(); }
};

template <typename _Tp>
class NumaVector : public vector<_Tp, CxxNumaAllocator<_Tp>> {};

/*
template <typename _Tp>
class VectorWp
{
public:
    virtual void push_back(const _Tp &x) = 0;
    virtual void pop_back() = 0;
    virtual void reserve(size_t n) = 0;
    virtual _Tp &operator[](size_t n) = 0;
    virtual _Tp *begin() = 0;
    virtual _Tp *end() = 0;
    virtual size_t size() = 0;
};

#define VECTOR_WP_VFUNC                 \
    void push_back(const _Tp &x)        \
    {                                   \
        v.push_back(x);                 \
    }                                   \
    void pop_back()                     \
    {                                   \
        v.pop_back();                   \
    }                                   \
    _Tp &operator[](size_t n)           \
    {                                   \
        return v[n];                    \
    }                                   \
    virtual _Tp *begin()                \
    {                                   \
        return (_Tp *)&v[0];            \
    }                                   \
    virtual _Tp *end()                  \
    {                                   \
        return (_Tp *)&v[0] + v.size(); \
    }                                   \
    void reserve(size_t n)              \
    {                                   \
        v.reserve(n);                   \
    }                                   \
    size_t size()                       \
    {                                   \
        return v.size();                \
    }

template <typename _Tp>
class BaseVector : public VectorWp<_Tp>
{
public:
    vector<_Tp> v;
    VECTOR_WP_VFUNC
};

template <typename _Tp>
class NumaVector : public VectorWp<_Tp>
{
    vector<_Tp, CxxNumaAllocator<_Tp>> v;

public:
    VECTOR_WP_VFUNC
};
*/
// NOLINTEND