#pragma once

#include <new>

#include "BumpAllocatorBase.h"

class BumpAllocatorHeader final : public BumpAllocatorBase {
public:
    ~BumpAllocatorHeader() override { reset(); }

protected:
    void _alloc_new_block(size_t size, size_t alignment) override {
        auto bytes_to_allocate = _compute_bytes_to_alloc(size, alignment);
        auto ptr = _mmap(bytes_to_allocate);

        m_unused_bytes = bytes_to_allocate - sizeof(BlockHeader);
        m_cursor = static_cast<std::byte *>(ptr) + sizeof(BlockHeader);

        new(ptr) BlockHeader(bytes_to_allocate, m_cur_block);
        m_cur_block = static_cast<BlockHeader *>(ptr);
    }

    void *_block_start_from_header(BlockHeader &header) override {
        return &header;
    }

private:
    [[nodiscard]] size_t _compute_bytes_to_alloc(size_t size, size_t alignment) const {
        size_t padding;
        if (page_size % alignment == 0)
            padding = (alignment - sizeof(BlockHeader) % alignment) % alignment;
        else
            padding = alignment - 1; // worst-case padding

        size_t bytes_required;
        if (__builtin_add_overflow(size, sizeof(BlockHeader), &bytes_required) or
            __builtin_add_overflow(bytes_required, padding, &bytes_required))
            throw std::bad_alloc();

        return _round_up_to_pagesize(bytes_required);
    }
};
