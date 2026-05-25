#pragma once

#include <cstddef>
#include <cstdint>

namespace switchui::android {

struct JitMemoryBlock {
    void* rw = nullptr;
    void* rx = nullptr;
    size_t size = 0;
    int fd = -1;
};

// Android replacement for iOS Mach JIT allocator.
// Creates dual mapping: RW alias for code generation + RX alias for execution.
JitMemoryBlock AllocateJitMemory(size_t size);
void ProtectJitAsExecutable(const JitMemoryBlock& block);
void ProtectJitAsWritable(const JitMemoryBlock& block);
void FlushJitInstructionCache(const void* start, size_t size);
void FreeJitMemory(JitMemoryBlock* block);

} // namespace switchui::android
