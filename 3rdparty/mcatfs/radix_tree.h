// NOLINTBEGIN
#pragma once
#include "bitmap.h"
#include "common.h"
#include "numa_tools.h"

#define TRIE_MAX_CHILD 11
#define TRIE_MAX_INLINE_SIZE (sizeof(uint32_t) * (TRIE_MAX_CHILD - 1))
#define TRIE_VAL_FLAG_BIT (1U << 31)

struct TrieNode {
  uint8_t inline_cnt, ch_cnt;
  char ch[TRIE_MAX_CHILD];
  char pad[3];
  uint32_t ch_idx[TRIE_MAX_CHILD];
  uint32_t val; // default -1;
} MCAT_PACKED;

struct Trie {
  NumaAllocator *allocator;
  Bitmap *alloc_bitmap;
  int64_t cnt, use_cnt;
  TrieNode node[0];
  TrieNode &index(int64_t idx) {
    p_assert(idx < cnt && idx >= 0);
    return node[idx];
  }
};

static_assert(sizeof(TrieNode) % sizeof(int32_t) == 0, "");
static_assert(offsetof(TrieNode, ch_idx) % 4 == 0, "");
static_assert(sizeof(TrieNode) == 64, "");

static inline Trie *trie_create(int64_t max_node, NumaAllocator &allocator,
                                int32_t numa_id) {
  p_assert(max_node < UINT32_MAX);
  int64_t alloc_size = sizeof(Trie) + max_node * sizeof(TrieNode);
  p_info("Trie Create, Max node %ld, %lfGB", max_node, (double)alloc_size / GB);
  Trie *trie = (Trie *)allocator.alloc(alloc_size, numa_id);
  trie->cnt = max_node;
  trie->use_cnt = 0;
  trie->allocator = &allocator;
  trie->alloc_bitmap = bitmap_create(max_node, allocator, numa_id);
  return trie;
}

static inline void trie_free(Trie *trie) {
  bitmap_free(trie->alloc_bitmap);
  trie->allocator->free(trie);
}

static inline uint32_t _trie_node_get_idx(TrieNode *node, Trie *trie) {
  return (uint32_t)(node - trie->node);
}

static inline void _trie_free_node(Trie *trie, TrieNode *node) {
  trie->use_cnt--;
  bitmap_reset(trie->alloc_bitmap, _trie_node_get_idx(node, trie));
}

static inline TrieNode *_trie_alloc_node(Trie *trie) {
  int64_t idx = bitmap_get(trie->alloc_bitmap);
  if (idx == -1)
    return NULL;
  p_assert(!(idx & TRIE_VAL_FLAG_BIT));
  TrieNode &ret = trie->index(idx);
  ret.inline_cnt = ret.ch_cnt = 0;
  ret.val = -1;
  trie->use_cnt++;
  return &ret;
}

static inline TrieNode *trie_alloc_root(Trie *trie) {
  return _trie_alloc_node(trie);
}

static inline char *_trie_node_get_inline(TrieNode *node) {
  return (char *)node->ch_idx + sizeof(node->ch_idx) - node->inline_cnt;
}

static inline bool _trie_node_is_substr(TrieNode *node, const char *str,
                                        int32_t len) {
  if (likely(node->inline_cnt == 0))
    return true;
  else if (unlikely(node->inline_cnt > len))
    return false;
  char *st = _trie_node_get_inline(node);
  return strncmp(str, st, node->inline_cnt) == 0 ? true : false;
}

static inline void _trie_node_split_case3(Trie *trie, TrieNode *node,
                                          int32_t idx) {
  TrieNode *new_node = _trie_alloc_node(trie);
  uint32_t val = node->ch_idx[idx];
  p_assert(val & TRIE_VAL_FLAG_BIT);
  new_node->val = val ^ TRIE_VAL_FLAG_BIT;
  node->ch_idx[idx] = _trie_node_get_idx(new_node, trie);
}

static inline void _trie_node_split_case2(Trie *trie, TrieNode *node) {
  int32_t inl_len = node->inline_cnt;
  int32_t next_inl_len = ALIGN_DOWN(inl_len - 1, sizeof(uint32_t));
  next_inl_len = std::max(next_inl_len, inl_len - next_inl_len - 1);
  TrieNode *new_node = _trie_alloc_node(trie);
  p_assert(new_node);
  *new_node = *node;

  char *st = _trie_node_get_inline(new_node);
  // init old_node
  node->inline_cnt = next_inl_len;
  memcpy(_trie_node_get_inline(node), st, next_inl_len);
  node->ch_cnt = 1;
  node->ch[0] = st[next_inl_len];
  node->ch_idx[0] = _trie_node_get_idx(new_node, trie);
  node->val = -1;
  // init new_node
  new_node->inline_cnt = inl_len - next_inl_len - 1;
}

static inline void _trie_node_split_case1(Trie *trie, TrieNode *node,
                                          const char *str, int32_t len) {
  int32_t min_len = std::min(len, (int32_t)node->inline_cnt);
  int32_t min_match = 0;

  TrieNode *new_node = _trie_alloc_node(trie);
  p_assert(new_node);
  *new_node = *node;

  char *st = _trie_node_get_inline(new_node);
  while (min_match < min_len) {
    if (str[min_match] != st[min_match])
      break;
    ++min_match;
  }

  int32_t new_inl_cnt = (int32_t)node->inline_cnt - min_match - 1;
  p_assert(new_inl_cnt >= 0);

  // init old node
  node->inline_cnt = min_match;
  memcpy(_trie_node_get_inline(node), st, min_match);
  node->ch_cnt = 1;
  node->ch[0] = st[min_match];
  node->val = -1;
  if (new_inl_cnt == 0 && new_node->ch_cnt == 0) { // inline val
    p_assert(new_node->val != (uint32_t)-1);
    p_assert(!(new_node->val & TRIE_VAL_FLAG_BIT));
    node->ch_idx[0] = new_node->val | TRIE_VAL_FLAG_BIT;
    _trie_free_node(trie, new_node);
  } else {
    node->ch_idx[0] = _trie_node_get_idx(new_node, trie);
    // init new node
    new_node->inline_cnt = new_inl_cnt;
  }
}

static inline int32_t _trie_node_free_ch_cnt(TrieNode *node) {
  int32_t ret =
      TRIE_MAX_CHILD - node->ch_cnt -
      ALIGN_UP((int32_t)node->inline_cnt, sizeof(uint32_t)) / sizeof(uint32_t);
  p_assert(ret >= 0);
  return ret;
}

// If key exists, it will not overwrite and return val
static inline uint32_t trie_insert(Trie *trie, TrieNode *root, const char *str,
                                   int32_t len, uint32_t val) {
  p_err_assert(!(val & TRIE_VAL_FLAG_BIT), "the last val bit is reserved");
  TrieNode *tmp = root;
  bool inl_val_flag = false;
  int32_t idx;
  while (len) {
    /*
        A node split occurs in two cases:
            1. Node inline string mismatch
            2. Node inlining leads to insufficient ch_idx
            3. inline val needs to be expanded into nodes
    */
    if (_trie_node_is_substr(tmp, str, len) == false) {
      _trie_node_split_case1(trie, tmp, str, len);
    }

    if (unlikely(tmp->val == ((uint32_t)-1) && tmp->ch_cnt == 0 &&
                 tmp->inline_cnt < TRIE_MAX_INLINE_SIZE &&
                 tmp->inline_cnt != len)) {
      // insert to inline
      tmp->inline_cnt = std::min(len, (int32_t)TRIE_MAX_INLINE_SIZE);
      memcpy(_trie_node_get_inline(tmp), str, tmp->inline_cnt);
    }

    if (len == tmp->inline_cnt)
      break;
    char c = str[tmp->inline_cnt];
    idx = 0;
    for (; idx < tmp->ch_cnt; idx++) {
      if (tmp->ch[idx] == c)
        break;
    }
    if (idx < tmp->ch_cnt) { // found
      uint32_t ch_idx = tmp->ch_idx[idx];
      if (ch_idx & TRIE_VAL_FLAG_BIT) {
        // inline val
        if (len == tmp->inline_cnt + 1) {
          inl_val_flag = true;
        } else {
          _trie_node_split_case3(trie, tmp, idx);
        }
      }
    } else { // insert
      int32_t free_cnt = _trie_node_free_ch_cnt(tmp);
      if (free_cnt == 0) {
        p_err_assert(tmp->inline_cnt != 0, "Child not enough");
        _trie_node_split_case2(trie, tmp);
        idx = 0;
      } else {
        if (len == tmp->inline_cnt + 1) {
          // inline val
          tmp->ch[tmp->ch_cnt] = c;
          tmp->ch_idx[tmp->ch_cnt] = val | TRIE_VAL_FLAG_BIT;
          idx = tmp->ch_cnt;
          ++tmp->ch_cnt;
          inl_val_flag = true;
        } else {
          TrieNode *new_node = _trie_alloc_node(trie);
          p_assert(new_node);
          tmp->ch[tmp->ch_cnt] = c;
          tmp->ch_idx[tmp->ch_cnt] = _trie_node_get_idx(new_node, trie);
          idx = tmp->ch_cnt;
          ++tmp->ch_cnt;

          const char *new_str = str + tmp->inline_cnt + 1;
          int32_t new_len = len - tmp->inline_cnt - 1;
          p_assert(new_len >= 0);
          new_len = std::min(new_len, (int32_t)TRIE_MAX_INLINE_SIZE);
          new_node->inline_cnt = new_len;
          memcpy(_trie_node_get_inline(new_node), new_str, new_len);
        }
      }
    }
    str += tmp->inline_cnt + 1;
    len -= tmp->inline_cnt + 1;
    p_assert(len >= 0);
    if (inl_val_flag == false) {
      tmp = &(trie->index(tmp->ch_idx[idx]));
    } else {
      p_assert(len == 0);
    }
  }

  if (inl_val_flag == false) {
    if (tmp->val == (uint32_t)-1) {
      tmp->val = val;
    }
    return tmp->val;
  } else {
    uint32_t inl_val = tmp->ch_idx[idx];
    p_assert(inl_val & TRIE_VAL_FLAG_BIT);
    return inl_val ^ TRIE_VAL_FLAG_BIT;
  }
}

// -1 if no found
static inline uint32_t trie_get(Trie *trie, TrieNode *root, const char *str,
                                int32_t len) {
  TrieNode *tmp = root;
  bool inl_val_flag = false;
  int32_t idx;
  while (len) {
    if (_trie_node_is_substr(tmp, str, len) == false)
      return -1;
    if (tmp->inline_cnt == len)
      break;
    idx = 0;
    char c = str[tmp->inline_cnt];
    for (; idx < tmp->ch_cnt; idx++) {
      if (tmp->ch[idx] == c)
        break;
    }
    if (idx < tmp->ch_cnt) { // found
      str += tmp->inline_cnt + 1;
      len -= tmp->inline_cnt + 1;
      uint32_t val = tmp->ch_idx[idx];
      if (val & TRIE_VAL_FLAG_BIT) {
        if (len) {
          return -1;
        } else {
          inl_val_flag = true;
        }
      } else {
        tmp = &(trie->index(tmp->ch_idx[idx]));
      }
    } else {
      return -1;
    }
  }
  if (inl_val_flag) {
    uint32_t ret = tmp->ch_idx[idx];
    p_assert(ret & TRIE_VAL_FLAG_BIT);
    return ret ^ TRIE_VAL_FLAG_BIT;
  } else {
    return tmp->val;
  }
}

static inline bool _trie_del_inner(Trie *trie, TrieNode *node, const char *str,
                                   int32_t len, uint32_t &val) {
  p_assert(len >= 0);
  if (len == 0) {
    val = node->val;
    node->val = -1;
    return (node->ch_cnt == 0);
  }
  if (_trie_node_is_substr(node, str, len) == false) {
    val = -1;
  } else {
    if (node->inline_cnt == len) {
      val = node->val;
      node->val = -1;
      return (node->ch_cnt == 0);
    }
    int32_t idx = 0;
    char c = str[node->inline_cnt];
    for (; idx < node->ch_cnt; idx++) {
      if (c == node->ch[idx]) {
        break;
      }
    }

    if (idx < node->ch_cnt) { // found
      uint32_t raw_val = node->ch_idx[idx];
      if ((raw_val & TRIE_VAL_FLAG_BIT) && len != node->inline_cnt + 1) {
        val = -1;
      } else {
        bool b;
        if (raw_val & TRIE_VAL_FLAG_BIT) {
          b = true;
          val = raw_val ^ TRIE_VAL_FLAG_BIT;
        } else {
          TrieNode *next_node = &(trie->index(node->ch_idx[idx]));
          b = _trie_del_inner(trie, next_node, str + node->inline_cnt + 1,
                              len - node->inline_cnt - 1, val);
          if (b)
            _trie_free_node(trie, next_node);
        }
        if (b) {
          --node->ch_cnt;
          node->ch[idx] = node->ch[node->ch_cnt];
          node->ch_idx[idx] = node->ch_idx[node->ch_cnt];
        }
      }
    } else {
      val = -1;
    }
  }
  return (node->ch_cnt == 0) && (node->val == (uint32_t)-1);
}

// return val
static inline uint32_t trie_del(Trie *trie, TrieNode *root, const char *str,
                                int32_t len) {
  uint32_t val;
  _trie_del_inner(trie, root, str, len, val);
  return val;
}

struct _TrieEdg {
  uint32_t a, b;
  char c;
};
static inline void _trie_dump_dfs(Trie *trie, TrieNode *node,
                                  vector<_TrieEdg> &ev, vector<uint32_t> &v) {
  int32_t idx = _trie_node_get_idx(node, trie);
  v.push_back(idx);
  for (int32_t i = 0; i < node->ch_cnt; i++) {
    _TrieEdg te;
    te.a = idx;
    te.b = node->ch_idx[i];
    te.c = node->ch[i];
    ev.push_back(te);
    if (!(te.b & TRIE_VAL_FLAG_BIT))
      _trie_dump_dfs(trie, &(trie->index(te.b)), ev, v);
  }
}

static inline string trie_dump(Trie *trie, TrieNode *root) {
  string s;
  std::stringstream ss;
  vector<_TrieEdg> ev;
  vector<uint32_t> v;
  _trie_dump_dfs(trie, root, ev, v);
  ss << v.size() << endl;
  for (auto i : v) {
    TrieNode *node = &(trie->index(i));
    ss << i << " "
       << (node->inline_cnt
               ? string(_trie_node_get_inline(node), (int32_t)node->inline_cnt)
               : string("_"))
       << " " << ((int32_t)node->val) << endl;
  }
  ss << ev.size() << endl;
  for (auto &i : ev) {
    ss << i.a << " " << i.b << " " << i.c << endl;
  }
  s = ss.str();
  return s;
}

static inline void trie_print_use(Trie *trie) {
  p_info("Trie Use %ld/%ld, %lfGB/%lfGB", trie->use_cnt, trie->cnt,
         (double)trie->use_cnt * 64 / GB, (double)trie->cnt * 64 / GB);
}

static inline int64_t trie_get_used_node(Trie *trie) { return trie->use_cnt; }
// NOLINTEND