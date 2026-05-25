#include "jit_memory.hpp"
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <android/log.h>
#include <errno.h>
#include <string.h>

#define LOG_TAG "SwitchUI_Memory"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace SwitchUI {

void* JitMemory::AllocateExecutable(size_t size) {
    // Android requires specific handling for JIT memory.
    // PROT_READ | PROT_WRITE | PROT_EXEC might be restricted on newer Android versions
    // without specific platform signing, but for NDK it's generally allowed in app context.
    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE | PROT_EXEC,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (ptr == MAP_FAILED) {
        LOGE("mmap failed: %s", strerror(errno));
        return nullptr;
    }
    return ptr;
}

bool JitMemory::Free(void* ptr, size_t size) {
    if (munmap(ptr, size) != 0) {
        LOGE("munmap failed: %s", strerror(errno));
        return false;
    }
    return true;
}

bool JitMemory::Protect(void* ptr, size_t size, bool executable) {
    int prot = PROT_READ | PROT_WRITE;
    if (executable) prot |= PROT_EXEC;

    if (mprotect(ptr, size, prot) != 0) {
        LOGE("mprotect failed: %s", strerror(errno));
        return false;
    }
    return true;
}

JitMemory::DualMapping JitMemory::AllocateDual(size_t size) {
    // For high-performance JIT, we use memfd_create to create a shared memory file
    // and map it twice: once RW and once RX.

    // Note: memfd_create is available from API 26.
    int fd = memfd_create("jit_memory", MFD_CLOEXEC);
    if (fd == -1) {
        LOGE("memfd_create failed: %s", strerror(errno));
        return {nullptr, nullptr};
    }

    if (ftruncate(fd, size) == -1) {
        LOGE("ftruncate failed: %s", strerror(errno));
        close(fd);
        return {nullptr, nullptr};
    }

    void* rw_ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    void* rx_ptr = mmap(nullptr, size, PROT_READ | PROT_EXEC, MAP_SHARED, fd, 0);

    close(fd); // FDs are no longer needed after mmap

    if (rw_ptr == MAP_FAILED || rx_ptr == MAP_FAILED) {
        LOGE("Dual mmap failed");
        if (rw_ptr != MAP_FAILED) munmap(rw_ptr, size);
        if (rx_ptr != MAP_FAILED) munmap(rx_ptr, size);
        return {nullptr, nullptr};
    }

    return {rw_ptr, rx_ptr};
}

} // namespace SwitchUI
