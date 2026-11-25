#pragma once

#include <stdexcept>
#include <unistd.h>

inline size_t ceil_div(size_t a, size_t b) {
    return a / b + (a % b != 0);
}

inline size_t get_page_size() {
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0)
        throw std::runtime_error("failed to get page size");
    return page_size;
}
