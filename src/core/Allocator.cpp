
#include <Spore/Internal.h>
#include <Spore/GeneralAllocator.h>
#include <memory>

GeneralAllocator* GeneralAllocator::Get() {
    return *(GeneralAllocator**)GetAddress(Internal, Allocator_ptr);
}

void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line) {
    if (*(uint32_t*)GetAddress(Internal, Allocator_ptr) == NULL) {
        return malloc(size);
    } else {
        return ((void* (*)(size_t, const char*, int, unsigned, const char*, int))GetAddress(Internal, new_))(
            size, pName, flags, debugFlags, file, line);
    }
}

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags,
                     unsigned debugFlags, const char* file, int line) {
    if (*(uint32_t*)GetAddress(Internal, Allocator_ptr) == NULL) {
        return malloc(size);
    } else {
        return ((void* (*)(size_t, const char*, int, unsigned, const char*, int))GetAddress(Internal, new_))(
            size, pName, flags, debugFlags, file, line);
    }
}

void operator delete[](void* p) noexcept {
    if (*(uint32_t*)GetAddress(Internal, Allocator_ptr) == NULL) {
        free(p);
    } else {
        ((void (*)(void*))GetAddress(Internal, delete_))(p);
    }
}

void* operator new(size_t n) {
    return operator new[](n, "", 0, 0, __FILE__, __LINE__);
}

void operator delete(void* p) noexcept {
    operator delete[](p);
}

void operator delete(void* p, size_t) noexcept {
    operator delete[](p);
}

void* operator new[](size_t n) {
    return operator new[](n, "", 0, 0, __FILE__, __LINE__);
}

