#pragma once
#include <cstdlib>
#include <new>

// One opt-in allocation failure on the calling test thread. Other threads and
// all ordinary allocations keep malloc/free behavior; no process is hooked.
namespace allocation_fixture {
struct State {
    bool armed = false;
    size_t requestedBytes = 0; // Zero selects the next allocation of any size.
    unsigned failures = 0;
    size_t failedBytes = 0;
};
inline thread_local State state;
struct Scope {
    State previous = state;
    explicit Scope(size_t bytes = 0) { state = {true, bytes, 0, 0}; }
    ~Scope() { state = previous; }
    bool Triggered() const { return state.failures == 1; }
};
}

void* operator new(std::size_t bytes) {
    auto& fault = allocation_fixture::state;
    if (fault.armed && (!fault.requestedBytes || fault.requestedBytes == bytes)) {
        fault.armed = false; // Error reporting and cleanup must be able to allocate.
        ++fault.failures;
        fault.failedBytes = bytes;
        throw std::bad_alloc();
    }
    if (void* memory = std::malloc(bytes ? bytes : 1)) return memory;
    throw std::bad_alloc();
}
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
