
#pragma once

#include <cstdint>

namespace CivText {
#include "generated/Strings.hpp"

void Bind(uintptr_t localeManagerGetter);
unsigned Active();
const char16_t* Get(Text id);
const char16_t* Format(Text id, int value, char16_t* buffer, unsigned size);
float CaptionWidth(unsigned section);
float BadgeWidth();
}

