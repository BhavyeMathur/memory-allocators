#pragma once

#include <memory>
#include <stdexcept>

#include "util.h"
#include "AllocatorInterface.h"

class BumpAllocatorBase : public Allocator {
public:
    void reset() {
        while (m_cur_block != nullptr) {
            auto prev_block = m_cur_block->prev_block;

            auto block_start = _block_start_from_header(*m_cur_block);
            if (munmap(block_start, m_cur_block->block_size))
                throw std::runtime_error("failed to unmap memory block");
            m_cur_block = prev_block;
        }

        m_cursor = nullptr;
        m_unused_bytes = 0;
    }

    void free(void *ptr, size_t len) override {
    }

protected:
    struct BlockHeader {
        size_t block_size;
        BlockHeader *prev_block;
    };

    BlockHeader *m_cur_block = nullptr;

    void *m_cursor = nullptr;
    size_t m_unused_bytes = 0;

    virtual void _alloc_new_block(size_t size, size_t alignment) = 0;

    virtual void *_block_start_from_header(BlockHeader &header) = 0;

private:
    void *_alloc(size_t size, size_t alignment) override {
        if (alignment == 0 or (alignment & alignment - 1) != 0)
            throw std::invalid_argument("alignment must be a power of two");

        while (!m_cursor or !std::align(alignment, size, m_cursor, m_unused_bytes))
            _alloc_new_block(size, alignment);

        auto ptr = m_cursor;
        m_cursor = static_cast<std::byte *>(m_cursor) + size;
        m_unused_bytes -= size;
        return ptr;
    }
};
