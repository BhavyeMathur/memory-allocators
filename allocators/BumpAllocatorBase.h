#pragma once

#include <memory>
#include <new>
#include <stdexcept>
#include <sys/mman.h>

#include "util.h"

class BumpAllocatorBase {
public:
    BumpAllocatorBase() : page_size(get_page_size()) {
    }

    BumpAllocatorBase(const BumpAllocatorBase &) = delete;

    BumpAllocatorBase &operator=(const BumpAllocatorBase &) = delete;

    virtual ~BumpAllocatorBase() = default;

    void *alloc(size_t size, size_t alignment = alignof(std::max_align_t)) {
        if (alignment == 0 or (alignment & alignment - 1) != 0)
            throw std::invalid_argument("alignment must be a power of two");

        while (!m_cursor or !std::align(alignment, size, m_cursor, m_unused_bytes))
            _alloc_new_block(size, alignment);

        auto ptr = m_cursor;
        m_cursor = static_cast<std::byte *>(m_cursor) + size;
        m_unused_bytes -= size;
        return ptr;
    }

    void *calloc(size_t size, size_t alignment = alignof(std::max_align_t)) {
        auto ptr = alloc(size, alignment);
        if (!ptr) return nullptr;

        memset(ptr, 0, size);
        return ptr;
    }

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

    static void free(void *) {
    }

protected:
    const size_t page_size;

    struct BlockHeader {
        size_t block_size;
        BlockHeader *prev_block;
    };

    BlockHeader *m_cur_block = nullptr;

    void *m_cursor = nullptr;
    size_t m_unused_bytes = 0;

    virtual void _alloc_new_block(size_t size, size_t alignment) = 0;

    [[nodiscard]] size_t _round_up_to_pagesize(size_t n) const {
        auto n_div_pagesize = ceil_div(n, page_size);

        size_t result;
        if (__builtin_mul_overflow(n_div_pagesize, page_size, &result))
            throw std::bad_alloc();

        return result;
    }

    virtual void *_block_start_from_header(BlockHeader &header) = 0;

    static void *_mmap(size_t bytes) {
        auto ptr = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, 0, 0);
        if (ptr == MAP_FAILED)
            throw std::bad_alloc();

        return ptr;
    }
};
