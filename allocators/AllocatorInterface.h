#pragma once

#include <sys/mman.h>


class Allocator {
public:
    Allocator() : page_size(get_page_size()) {
    }

    Allocator(const Allocator &) = delete;

    Allocator &operator=(const Allocator &) = delete;

    virtual ~Allocator() = default;

    void *alloc(size_t size, size_t alignment = alignof(std::max_align_t)) {
        return _alloc(size, alignment);
    }

    void *calloc(size_t size, size_t alignment = alignof(std::max_align_t)) {
        auto ptr = alloc(size, alignment);
        if (!ptr) return nullptr;

        memset(ptr, 0, size);
        return ptr;
    }

    virtual void free(void *ptr, size_t len) = 0;

protected:
    const size_t page_size;

    static void *_mmap(size_t bytes) {
        auto ptr = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, 0, 0);
        if (ptr == MAP_FAILED)
            throw std::bad_alloc();

        return ptr;
    }

private:
    virtual void *_alloc(size_t size, size_t alignment) = 0;
};
