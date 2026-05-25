#pragma once

#include <cstddef>
#include <sys/mman.h>

namespace SwitchUI {

class JitMemory {
public:
    static void* AllocateExecutable(size_t size);
    static bool Free(void* ptr, size_t size);
    static bool Protect(void* ptr, size_t size, bool executable);

    // iOS often uses dual mapping (one RW, one RX) for JIT.
    // On Android, we can simulate this or use simple mmap with PROT_FLAGS.
    struct DualMapping {
        void* rw_ptr;
        void* rx_ptr;
    };

    static DualMapping AllocateDual(size_t size);
};

} // namespace SwitchUI
