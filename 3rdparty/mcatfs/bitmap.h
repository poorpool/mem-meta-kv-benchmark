// NOLINTBEGIN
#pragma once
#include "common.h"
#include "numa_tools.h"
struct Bitmap
{
    int64_t size;
    int64_t ptr;
    NumaAllocator *allocator;
    uint64_t buf[0];
};

static inline Bitmap *bitmap_create(int64_t cnt, NumaAllocator &allocator, int32_t numa_id)
{
    int64_t alloc_size = sizeof(Bitmap) + ALIGN_UP(cnt, 64) / 8;
    Bitmap *bm = (Bitmap *)allocator.alloc(alloc_size, numa_id);
    memset(bm, 0, alloc_size);
    bm->size = cnt;
    bm->ptr = 0;
    bm->allocator = &allocator;
    return bm;
}

static inline void bitmap_free(Bitmap *bm)
{
    bm->allocator->free(bm);
}

static inline void bitmap_set(Bitmap *bm, int64_t idx)
{
    p_assert(idx >= 0 && idx < bm->size);
    p_assert(!(bm->buf[idx / 64] & (1UL << (idx & 63))));
    bm->buf[idx / 64] |= 1UL << (idx & 63);
}

static inline void bitmap_reset(Bitmap *bm, int64_t idx)
{
    p_assert(idx >= 0 && idx < bm->size);
    p_assert((bm->buf[idx / 64] & (1UL << (idx & 63))));
    bm->buf[idx / 64] &= ~(1UL << (idx & 63));
}

static inline int64_t first_zero_of_ul(uint64_t val)
{
    return __builtin_ctzll(val + 1);
}

static inline int64_t bitmap_get(Bitmap *bm)
{
    int64_t buf_cnt = (bm->size + 63) >> 6;
    for (int64_t i = 0; i < buf_cnt; i++)
    {
        int64_t idx = (i + bm->ptr) % buf_cnt;
        auto &buf = bm->buf[idx];
        if (buf == (uint64_t)-1)
            continue;
        int64_t k = first_zero_of_ul(bm->buf[idx]) + (idx << 6);
        if (k >= bm->size)
            continue;
        // find a free one
        bm->ptr = idx;
        bitmap_set(bm, k);
        return k;
    }
    return -1;
}
// NOLINTEND