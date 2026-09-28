
#include <Windows.h>
#include <Spore/ModAPI.h>
#include "core/SdkCompat.hpp"
#include <Spore/App/cLocaleManager.h>
#define CIV_TEXT_TABLES
#include "localization/Localization.hpp"

namespace CivText {
namespace {
using LocaleManagerFn = App::cLocaleManager* (*)();
LocaleManagerFn localeManager;

char16_t Lower(char16_t c) {
    return c >= u'A' && c <= u'Z' ? char16_t(c - u'A' + u'a') : c == u'_' ? u'-' : c;
}

bool Match(const char16_t* code, unsigned length, unsigned& index) {
    if (length < 2 || (length > 2 && code[2] != u'-' && code[2] != u'_'))
        return false;
    for (unsigned compare : {5u, 2u}) {
        if (compare > length || (compare == 5 && length > 5 && code[5] != u'-' && code[5] != u'_' && code[5] != u'.'))
            continue;
        for (unsigned i = 0; i < LocaleCount; ++i) {
            unsigned c = 0;
            while (c < compare && Lower(code[c]) == char16_t(LocaleCodes[i][c]))
                ++c;
            if (c == compare) {
                index = i;
                return true;
            }
        }
    }
    return false;
}

unsigned FromCommandLine() {
    const wchar_t* line = GetCommandLineW();
    for (const wchar_t* c = line; c && *c; ++c) {
        if ((*c != L'-' && *c != L'/') || (c > line && c[-1] != L' ' && c[-1] != L'"'))
            continue;
        const wchar_t* key = L"locale:";
        unsigned k = 0;
        while (key[k] && Lower(char16_t(c[1 + k])) == char16_t(key[k]))
            ++k;
        if (key[k])
            continue;
        const wchar_t* value = c + 1 + k;
        if (*value == L'"')
            ++value;
        unsigned length = 0;
        while (value[length] && value[length] != L'"' && value[length] != L' ' && length < 8)
            ++length;
        unsigned index = 0;
        if (Match(reinterpret_cast<const char16_t*>(value), length, index))
            return index;
    }
    return 0;
}
}

void Bind(uintptr_t localeManagerGetter) {
    localeManager = reinterpret_cast<LocaleManagerFn>(localeManagerGetter);
}

unsigned Active() {
    auto* manager = localeManager ? localeManager() : nullptr;
    if (!manager)
        return localeManager ? 0 : FromCommandLine();
    const auto& code = manager->GetActiveLanguage();
    unsigned index = 0;
    return Match(code.c_str(), unsigned(code.size()), index) ? index : 0;
}

const char16_t* Get(Text id) {
    return id < TextCount ? Strings[Active()][id] : u"";
}

const char16_t* Format(Text id, int value, char16_t* buffer, unsigned size) {
    if (!buffer || !size)
        return u"";
    char16_t digits[12]{};
    unsigned count = 0;
    unsigned magnitude = value < 0 ? unsigned(-(value + 1)) + 1 : unsigned(value);
    do {
        digits[count++] = char16_t(u'0' + magnitude % 10);
        magnitude /= 10;
    } while (magnitude && count < 11);
    if (value < 0)
        digits[count++] = u'-';
    unsigned length = 0;
    for (const char16_t* c = Get(id); *c && length + 1 < size; ++c) {
        if (*c != u'#') {
            buffer[length++] = *c;
            continue;
        }
        for (unsigned d = count; d > 0 && length + 1 < size; --d)
            buffer[length++] = digits[d - 1];
    }
    buffer[length] = 0;
    return buffer;
}

float CaptionWidth(unsigned section) {
    return section < 3 ? float(CaptionWidths[Active()][section]) : 0;
}

float BadgeWidth() {
    return float(BadgeWidths[Active()]);
}
}

