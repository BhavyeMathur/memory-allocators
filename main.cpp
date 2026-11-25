#include <cassert>
#include <iostream>

#include "allocators/allocators.h"

int main() {
    BumpAllocator allocator;

    {
        auto ptr1 = static_cast<uint8_t *>(allocator.alloc(1024));
        memset(ptr1, 1, 1024);

        auto ptr2 = static_cast<uint8_t *>(allocator.alloc(1024));
        memset(ptr2, 2, 1024);

        auto ptr3 = static_cast<uint8_t *>(allocator.alloc(65536));
        memset(ptr3, 3, 65536);

        auto ptr4 = static_cast<uint8_t *>(allocator.alloc(4, 2));
        memset(ptr4, 4, 4);

        auto ptr5 = static_cast<uint8_t *>(allocator.alloc(4, 32));
        memset(ptr5, 5, 4);

        assert(reinterpret_cast<uintptr_t>(ptr1) % alignof(std::max_align_t) == 0);
        assert(reinterpret_cast<uintptr_t>(ptr2) % alignof(std::max_align_t) == 0);
        assert(reinterpret_cast<uintptr_t>(ptr3) % alignof(std::max_align_t) == 0);
        assert(reinterpret_cast<uintptr_t>(ptr4) % 2 == 0);
        assert(reinterpret_cast<uintptr_t>(ptr5) % 32 == 0);

        for (int i = 0; i < 1024; i++) {
            assert(ptr1[i] == 1);
            assert(ptr2[i] == 2);
        }

        for (int i = 0; i < 65536; i++)
            assert(ptr3[i] == 3);

        for (int i = 0; i < 4; i++) {
            assert(ptr4[i] == 4);
            assert(ptr5[i] == 5);
        }

        std::cout << "Basic tests passed" << std::endl;
    }

    allocator.reset();

    {
        auto ptr6 = static_cast<uint8_t *>(allocator.calloc(32, 32));
        assert(reinterpret_cast<uintptr_t>(ptr6) % 32 == 0);

        auto ptr7 = static_cast<uint8_t *>(allocator.calloc(1024));
        assert(reinterpret_cast<uintptr_t>(ptr7) % alignof(std::max_align_t) == 0);

        for (int i = 0; i < 32; i++)
            assert(ptr6[i] == 0);

        for (int i = 0; i < 1024; i++)
            assert(ptr7[i] == 0);

        std::cout << "Calloc tests passed" << std::endl;
    }

    allocator.reset();
    allocator.reset();

    {
        try {
            allocator.alloc(1024, 3);
            assert(false);
        } catch (std::invalid_argument &) {}

        try {
            allocator.alloc(1024, 0);
            assert(false);
        } catch (std::invalid_argument &) {}

        try {
            allocator.alloc(SIZE_MAX);
            assert(false);
        } catch (std::bad_alloc &) {}

        std::cout << "Edge case tests passed" << std::endl;
    }

    allocator.reset();

    {
        auto ptr8 = static_cast<uint8_t *>(allocator.alloc(1024, 65536));
        memset(ptr8, 8, 1024);

        assert(reinterpret_cast<uintptr_t>(ptr8) % 65536 == 0);
        for (int i = 0; i < 1024; i++)
            assert(ptr8[i] == 8);

        std::cout << "Large alignment test passed" << std::endl;
    }

    std::cout << "\nAll tests passed!" << std::endl;

    return 0;
}
