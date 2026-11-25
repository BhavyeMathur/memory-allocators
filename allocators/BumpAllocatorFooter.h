#pragma once

#include <new>

#include "BumpAllocatorBase.h"

class BumpAllocatorFooter final : public BumpAllocatorBase {
public:
    ~BumpAllocatorFooter() override { reset(); }

protected:
    void _alloc_new_block(size_t size, size_t alignment) override {
        auto bytes_to_allocate = _compute_bytes_to_alloc(size, alignment);
        auto ptr = _mmap(bytes_to_allocate);

        m_unused_bytes = bytes_to_allocate - sizeof(BlockHeader);
        m_cursor = ptr;

        auto new_header = static_cast<std::byte *>(m_cursor) + bytes_to_allocate - sizeof(BlockHeader);
        new(new_header) BlockHeader(bytes_to_allocate, m_cur_block);
        m_cur_block = reinterpret_cast<BlockHeader *>(new_header);
    }

    void *_block_start_from_header(BlockHeader &header) override {
        return reinterpret_cast<std::byte *>(m_cur_block) + sizeof(BlockHeader) - m_cur_block->block_size;
    }

private:
    [[nodiscard]] size_t _compute_bytes_to_alloc(size_t size, size_t alignment) const {
        size_t padding;
        if (page_size % alignment == 0)
            padding = 0;
        else
            padding = alignment - 1; // worst-case padding

        size_t bytes_required;
        if (__builtin_add_overflow(size, sizeof(BlockHeader), &bytes_required) or
            __builtin_add_overflow(bytes_required, padding, &bytes_required))
            throw std::bad_alloc();

        return _round_up_to_pagesize(bytes_required);
    }
};
