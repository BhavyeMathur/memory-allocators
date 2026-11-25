#pragma once

#include <stdexcept>
#include <unistd.h>

template<typename T>
T ceil_div(T a, T b) {
    return a / b + (a % b != 0);
}

template<typename T>
T round_up_to_multiple(T a, T b) {
    auto a_div_b = ceil_div(a, b);

    T result;
    if (__builtin_mul_overflow(a_div_b, b, &result))
        throw std::invalid_argument("overflow");

    return result;
}

inline size_t mapping_alignment_padding(size_t alignment, size_t page_size, size_t payload_offset) {
    if (page_size % alignment == 0)
        return (alignment - payload_offset % alignment) % alignment;
    return alignment - 1; // worst-case padding
}

inline size_t required_mapping_size(size_t size, size_t alignment, size_t page_size,
                                    size_t payload_offset, size_t payload_size) {
    size_t padding = mapping_alignment_padding(alignment, page_size, payload_offset);

    size_t bytes_required;
    if (__builtin_add_overflow(size, padding, &bytes_required) or
        __builtin_add_overflow(bytes_required, payload_size, &bytes_required))
        throw std::invalid_argument("overflow");

    return round_up_to_multiple(bytes_required, page_size);
}

inline size_t get_page_size() {
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0)
        throw std::runtime_error("failed to get page size");
    return page_size;
}
