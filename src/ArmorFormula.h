#pragma once

#include "Settings.h"

namespace MAF
{
    [[nodiscard]] float MorrowindDamageMultiplier(float armorRating, const Settings& settings);
    [[nodiscard]] float GetArmorRating(RE::Actor& target, const Settings& settings);
    [[nodiscard]] float AdjustHealthDamage(RE::Actor& target, RE::Actor* attacker, float incomingDamage, const Settings& settings);
}
