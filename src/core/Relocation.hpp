
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

// The game's executables are relocatable. Wine maps them at their preferred base, but Windows with ASLR maps them at
// another one, and the loader then adds the difference to every dword the .reloc directory lists, in the code too.
// The expected bytes in Hooks.hpp are the file's, so a check takes that difference back out before it compares.
namespace Relocation {
constexpr uint32_t PreferredBase = 0x400000;
constexpr size_t MaxCheck = 32;

inline uint32_t Dword(const unsigned char* p) {
    uint32_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

// image is the mapped image, delta its base minus PreferredBase. At delta 0 this is the plain compare. A moved image
// without a readable relocation directory never matches.
inline bool Matches(const unsigned char* image, uint32_t delta, uint32_t rva, const char* expected, size_t size) {
    if (!delta)
        return !memcmp(image + rva, expected, size);
    if (size > MaxCheck)
        return false;
    unsigned char bytes[MaxCheck];
    memcpy(bytes, image + rva, size);
    const unsigned char* nt = image + Dword(image + 0x3c);
    uint32_t imageSize = Dword(nt + 0x50), table = Dword(nt + 0xa0), tableSize = Dword(nt + 0xa4);
    if (!table || tableSize < 8 || table > imageSize || tableSize > imageSize - table)
        return false;
    uint32_t end = rva + uint32_t(size);
    for (uint32_t at = 0; at + 8 <= tableSize;) {
        const unsigned char* block = image + table + at;
        uint32_t page = Dword(block), blockSize = Dword(block + 4);
        if (blockSize < 8 || blockSize % 2 || blockSize > tableSize - at)
            return false;
        at += blockSize;
        if (page >= end || page + 0x1000 + 3 <= rva)
            continue;
        for (uint32_t i = 8; i < blockSize; i += 2) {
            uint16_t entry;
            memcpy(&entry, block + i, sizeof(entry));
            if (entry >> 12 == 0)
                continue;
            if (entry >> 12 != 3)
                return false;
            uint32_t target = page + (entry & 0xfff);
            if (target + 4 <= rva || target >= end)
                continue;
            uint32_t original = Dword(image + target) - delta;
            for (uint32_t k = 0; k < 4; ++k)
                if (target + k >= rva && target + k < end)
                    bytes[target + k - rva] = uint8_t(original >> 8 * k);
        }
    }
    return !memcmp(bytes, expected, size);
}
}

