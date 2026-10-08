
#pragma once
#include <Windows.h>
#include <cstdint>
#include <cstring>

namespace HookChain {
inline uintptr_t ForeignTarget(const void* code, uintptr_t hostBase, size_t hostSize) {
    auto* p = static_cast<const unsigned char*>(code);
    if (p[0] != 0xe9)
        return 0;
    int32_t offset;
    memcpy(&offset, p + 1, sizeof(offset));
    uintptr_t target = reinterpret_cast<uintptr_t>(p) + 5 + uintptr_t(offset);
    if (target >= hostBase && target - hostBase < hostSize)
        return 0;
    MEMORY_BASIC_INFORMATION info{};
    if (!VirtualQuery(reinterpret_cast<void*>(target), &info, sizeof(info)) || info.State != MEM_COMMIT)
        return 0;
    constexpr DWORD executable = PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
    return info.Protect & executable ? target : 0;
}

inline bool JumpTo(void* code, void* destination) {
    auto* p = static_cast<unsigned char*>(code);
    DWORD old = 0;
    if (!VirtualProtect(p, 5, PAGE_EXECUTE_READWRITE, &old))
        return false;
    unsigned char jump[5] = {0xe9};
    int32_t offset = int32_t(reinterpret_cast<uintptr_t>(destination) - (reinterpret_cast<uintptr_t>(p) + 5));
    memcpy(jump + 1, &offset, sizeof(offset));
    memcpy(p, jump, sizeof(jump));
    VirtualProtect(p, 5, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 5);
    return true;
}
}

