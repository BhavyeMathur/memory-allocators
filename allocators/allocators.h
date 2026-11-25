#pragma once

#include <unistd.h>
#include <memory>
#include <new>
#include <stdexcept>
#include <sys/mman.h>


inline size_t ceil_div(size_t a, size_t b) {
    return a / b + (a % b != 0);
}

class BumpAllocator {
public:
    BumpAllocator() : page_size([] {
        long page_size = sysconf(_SC_PAGESIZE);
        if (page_size <= 0)
            throw std::runtime_error("failed to get page size");
        return page_size;
    }()) {
    }

    BumpAllocator(BumpAllocator &) = delete;

    BumpAllocator &operator=(BumpAllocator &) = delete;

    ~BumpAllocator() { reset(); }

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
            munmap(m_cur_block, m_cur_block->block_size);
            m_cur_block = prev_block;
        }

        m_cursor = nullptr;
        m_unused_bytes = 0;
    }

private:
    const size_t page_size;

    struct BlockHeader {
        size_t block_size;
        BlockHeader *prev_block;
    };

    BlockHeader *m_cur_block = nullptr;

    void *m_cursor = nullptr;
    size_t m_unused_bytes = 0;

    void _alloc_new_block(size_t size, size_t alignment) {
        auto bytes_to_allocate = _compute_bytes_to_alloc(size, alignment);

        auto ptr = mmap(nullptr, bytes_to_allocate, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, 0, 0);
        if (ptr == MAP_FAILED)
            throw std::bad_alloc();

        new(ptr) BlockHeader(bytes_to_allocate, m_cur_block);
        m_cur_block = static_cast<BlockHeader *>(ptr);

        m_cursor = static_cast<std::byte *>(ptr) + sizeof(BlockHeader);
        m_unused_bytes = bytes_to_allocate - sizeof(BlockHeader);
    }

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

        auto pages_required = ceil_div(bytes_required, page_size);

        size_t bytes_to_allocate;
        if (__builtin_mul_overflow(pages_required, page_size, &bytes_to_allocate))
            throw std::bad_alloc();

        return bytes_to_allocate;
    }
};
