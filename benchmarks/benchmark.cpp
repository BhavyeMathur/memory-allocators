#include <algorithm>
#include <chrono>
#include <iomanip>
#include <vector>
#include <iostream>
#include <sys/mman.h>

using Clock = std::chrono::steady_clock;
using Nanoseconds = std::chrono::nanoseconds;

uint64_t ns(Clock::duration duration) {
    return std::chrono::duration_cast<Nanoseconds>(duration).count();
}

struct Counters {
    uint64_t maps = 0;
    uint64_t unmaps = 0;

    uint64_t bytes_mapped = 0;
    uint64_t total_bytes_mapped = 0;
    uint64_t peak_bytes_mapped = 0;
} counters;

namespace bench {
    void *map(void *address, size_t length, int protection, int flags, int fd, off_t offset) {
        auto result = ::mmap(address, length, protection, flags, fd, offset);
        if (result != MAP_FAILED) {
            counters.bytes_mapped += length;
            counters.total_bytes_mapped += length;
            counters.peak_bytes_mapped = std::max(counters.peak_bytes_mapped, counters.bytes_mapped);
            counters.maps++;
        }
        return result;
    }

    int unmap(void *address, size_t length) {
        auto result = ::munmap(address, length);
        if (result == 0) {
            counters.bytes_mapped -= length;
            counters.unmaps++;
        }
        return result;
    }
}

#define mmap bench::map
#define munmap bench::unmap

#include "allocators/allocators.h"

struct Workload {
    struct Request {
        size_t size; // used by Alloc, Calloc
        size_t alignment; // used by Alloc, Calloc

        size_t ptr_index; // used by Free, Touch
        size_t offset; // used by Touch

        enum class Type { Alloc, Calloc, Reset, Free, Touch } type;
    };

    std::vector<Request> requests;
};

void print_results(const std::vector<uint64_t> &latency_vs_step, const std::vector<Counters> &counters_vs_step) {
    auto print_row = [](const auto &sample, const auto &latency,
                        const auto &maps, const auto &unmaps,
                        const auto &bytes, const auto &total,
                        const auto &peak) {
        std::cout << std::setw(8) << sample
                << std::setw(16) << latency
                << std::setw(8) << maps
                << std::setw(8) << unmaps
                << std::setw(18) << bytes
                << std::setw(20) << total
                << std::setw(20) << peak
                << '\n';
    };

    std::cout << std::left;
    print_row("i", "latency(ns)", "maps", "unmaps",
              "bytes_mapped", "total_bytes_mapped", "peak_bytes_mapped");

    for (size_t i = 0; i < latency_vs_step.size(); ++i) {
        const auto &c = counters_vs_step[i];
        print_row(i, latency_vs_step[i], c.maps, c.unmaps,
                  c.bytes_mapped, c.total_bytes_mapped, c.peak_bytes_mapped);
    }
}

void run(BumpAllocatorBase &allocator, Workload &workload) {
    std::vector<uint64_t> latency_vs_step;
    std::vector<Counters> counters_vs_step;
    std::vector<void *> ptrs;

    ptrs.reserve(workload.requests.size());
    latency_vs_step.reserve(workload.requests.size());
    counters_vs_step.reserve(workload.requests.size());

    for (auto [size, alignment, ptr_index, offset, type]: workload.requests) {
        void *ptr = nullptr;

        std::chrono::time_point<Clock> start, end;
        if (type != Workload::Request::Type::Touch)
            start = Clock::now();
        switch (type) {
            case Workload::Request::Type::Alloc:
                ptr = allocator.alloc(size, alignment);
                break;

            case Workload::Request::Type::Calloc:
                ptr = allocator.calloc(size, alignment);
                break;

            case Workload::Request::Type::Reset:
                allocator.reset();
                break;

            case Workload::Request::Type::Free:
                allocator.free(ptrs[ptr_index]);
                break;

            case Workload::Request::Type::Touch:
                static_cast<volatile uint8_t *>(ptrs[ptr_index])[offset] = 0;
                break;
        }
        if (type != Workload::Request::Type::Touch)
            end = Clock::now();

        if (ptr)
            ptrs.push_back(ptr);
        if (type == Workload::Request::Type::Touch)
            continue;
        latency_vs_step.push_back(ns(end - start));
        counters_vs_step.push_back(counters);
    }

    print_results(latency_vs_step, counters_vs_step);
}

int main() {
    Workload workload = {
        {
            Workload::Request(1024, 8, 0, 0, Workload::Request::Type::Alloc),
            Workload::Request(1024, 8, 0, 0, Workload::Request::Type::Alloc),
            Workload::Request(1024, 8, 0, 0, Workload::Request::Type::Alloc),
            Workload::Request(1024, 8, 0, 0, Workload::Request::Type::Alloc),
        }
    };

    BumpAllocator allocator;

    run(allocator, workload);
    return 0;
}
