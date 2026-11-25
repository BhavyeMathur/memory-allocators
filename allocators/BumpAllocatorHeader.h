#pragma once

#include "BumpAllocatorBase.h"

class BumpAllocatorHeader final : public BumpAllocatorBase {
public:
    ~BumpAllocatorHeader() override { reset(); }

protected:
    void _alloc_new_block(size_t size, size_t alignment) override {
        auto bytes_to_allocate = required_mapping_size(size, alignment, page_size,
                                                       sizeof(BlockHeader), sizeof(BlockHeader));
        auto ptr = _mmap(bytes_to_allocate);

        m_unused_bytes = bytes_to_allocate - sizeof(BlockHeader);
        m_cursor = static_cast<std::byte *>(ptr) + sizeof(BlockHeader);

        new(ptr) BlockHeader(bytes_to_allocate, m_cur_block);
        m_cur_block = static_cast<BlockHeader *>(ptr);
    }

    void *_block_start_from_header(BlockHeader &header) override {
        return &header;
    }
};
