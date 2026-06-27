#pragma once

namespace MAF
{
    enum class ArmorSource
    {
        kDisplayedDamageResist,
        kDisplayedPlusHiddenArmorSlots
    };

    struct Settings
    {
        bool enabled = true;

        bool affectPlayer = true;
        bool affectNPCs = true;
        bool affectCreatures = true;

        ArmorSource armorSource = ArmorSource::kDisplayedDamageResist;
        float percentHealthPerArmorPoint = 0.01F;
        float minDamageMultiplier = 0.0F;

        float hiddenArmorPerSlot = 25.0F;
        bool hiddenArmorHead = true;
        bool hiddenArmorBody = true;
        bool hiddenArmorHands = true;
        bool hiddenArmorFeet = true;

        float vanillaArmorReductionPerPoint = 0.0012F;
        float vanillaMaxReduction = 0.80F;

        bool requireRecentHitData = true;
        float staleHitDamageTolerance = 0.01F;
        bool logAdjustments = false;
    };

    [[nodiscard]] Settings& GetSettings();
    void LoadSettings();
}
