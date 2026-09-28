
#pragma once

#include <cstdint>

namespace CivSettings {
enum Rule : unsigned { LandRaids, Turrets, Spice, Superweapons, Compliments, Gifts, RuleCount };

extern bool rules[RuleCount];

inline bool Enabled(Rule rule) {
    return rules[rule];
}

void Load();
bool Set(unsigned rule, bool value);
void BindUI(uintptr_t base);
void UpdateUI();
}

