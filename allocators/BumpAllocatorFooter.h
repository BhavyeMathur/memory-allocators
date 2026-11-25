#pragma once

#include "BumpAllocatorBase.h"

class BumpAllocatorFooter final : public BumpAllocatorBase {
public:
    ~BumpAllocatorFooter() override { reset(); }

protected:
    void _alloc_new_block(size_t size, size_t alignment) override {
        auto bytes_to_allocate = required_mapping_size(size, alignment, page_size, 0, sizeof(BlockHeader));
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
};
