#include "jit_memory_android.h"

#include <android/api-level.h>
#include <android/log.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/ashmem.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <cstring>

namespace switchui::android {
namespace {

constexpr const char* kLogTag = "SwitchUI-JIT";

#if !defined(MFD_CLOEXEC)
#define MFD_CLOEXEC 0x0001U
#endif

#ifndef SYS_memfd_create
#if defined(__aarch64__)
#define SYS_memfd_create 279
#else
#define SYS_memfd_create 385
#endif
#endif

int CreateSharedJitFd(const char* name, size_t size) {
#if __ANDROID_API__ >= 30
    int fd = memfd_create(name, MFD_CLOEXEC);
#else
    int fd = static_cast<int>(syscall(SYS_memfd_create, name, MFD_CLOEXEC));
#endif
    if (fd >= 0) {
        if (ftruncate(fd, static_cast<off_t>(size)) == 0) {
            return fd;
        }
        close(fd);
    }

    // Fallback for old devices/toolchains: ashmem region.
    int ashmem_fd = open("/dev/ashmem", O_RDWR);
    if (ashmem_fd < 0) {
        return -1;
    }

    if (ioctl(ashmem_fd, ASHMEM_SET_NAME, name) < 0 ||
        ioctl(ashmem_fd, ASHMEM_SET_SIZE, size) < 0) {
        close(ashmem_fd);
        return -1;
    }

    return ashmem_fd;
}

size_t RoundToPageSize(size_t size) {
    const size_t page = static_cast<size_t>(sysconf(_SC_PAGESIZE));
    return (size + page - 1) & ~(page - 1);
}

} // namespace

JitMemoryBlock AllocateJitMemory(size_t size) {
    JitMemoryBlock block{};
    block.size = RoundToPageSize(size);
    block.fd = CreateSharedJitFd("switchui_jit", block.size);

    if (block.fd < 0) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "CreateSharedJitFd failed: %d", errno);
        return {};
    }

    block.rw = mmap(nullptr, block.size, PROT_READ | PROT_WRITE, MAP_SHARED, block.fd, 0);
    block.rx = mmap(nullptr, block.size, PROT_READ | PROT_EXEC, MAP_SHARED, block.fd, 0);

    if (block.rw == MAP_FAILED || block.rx == MAP_FAILED) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "mmap failed rw=%p rx=%p errno=%d", block.rw, block.rx, errno);
        if (block.rw != MAP_FAILED) {
            munmap(block.rw, block.size);
        }
        if (block.rx != MAP_FAILED) {
            munmap(block.rx, block.size);
        }
        close(block.fd);
        return {};
    }

    return block;
}

void ProtectJitAsExecutable(const JitMemoryBlock& block) {
    if (!block.rw || !block.size) {
        return;
    }
    mprotect(block.rw, block.size, PROT_READ);
    mprotect(block.rx, block.size, PROT_READ | PROT_EXEC);
}

void ProtectJitAsWritable(const JitMemoryBlock& block) {
    if (!block.rw || !block.size) {
        return;
    }
    mprotect(block.rw, block.size, PROT_READ | PROT_WRITE);
    mprotect(block.rx, block.size, PROT_READ);
}

void FlushJitInstructionCache(const void* start, size_t size) {
    auto* begin = reinterpret_cast<const char*>(start);
    auto* end = begin + size;
    __builtin___clear_cache(const_cast<char*>(begin), const_cast<char*>(end));
}

void FreeJitMemory(JitMemoryBlock* block) {
    if (!block) {
        return;
    }

    if (block->rw) {
        munmap(block->rw, block->size);
    }
    if (block->rx) {
        munmap(block->rx, block->size);
    }
    if (block->fd >= 0) {
        close(block->fd);
    }

    *block = {};
}

} // namespace switchui::android
