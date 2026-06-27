#include "ArmorFormula.h"

#include "PCH.h"

namespace
{
    using Slot = RE::BGSBipedObjectForm::BipedObjectSlot;

    [[nodiscard]] bool IsPlayer(RE::Actor& actor)
    {
        return &actor == RE::PlayerCharacter::GetSingleton();
    }

    [[nodiscard]] bool IsNpc(RE::Actor& actor)
    {
        return &actor != RE::PlayerCharacter::GetSingleton();
    }

    [[nodiscard]] bool ShouldAffectActor(RE::Actor& target, const MAF::Settings& settings)
    {
        if (!settings.enabled) {
            return false;
        }

        if (IsPlayer(target)) {
            return settings.affectPlayer;
        }

        if (IsNpc(target)) {
            return settings.affectNPCs;
        }

        return settings.affectCreatures;
    }

    [[nodiscard]] bool HasWornArmorInSlot(RE::Actor& target, Slot slot)
    {
        const auto* armor = target.GetWornArmor(slot);
        return armor && !armor->IsClothing();
    }

    [[nodiscard]] float HiddenArmorBonus(RE::Actor& target, const MAF::Settings& settings)
    {
        float slots = 0.0F;
        if (settings.hiddenArmorHead && HasWornArmorInSlot(target, Slot::kHead)) {
            slots += 1.0F;
        }
        if (settings.hiddenArmorBody && HasWornArmorInSlot(target, Slot::kBody)) {
            slots += 1.0F;
        }
        if (settings.hiddenArmorHands && HasWornArmorInSlot(target, Slot::kHands)) {
            slots += 1.0F;
        }
        if (settings.hiddenArmorFeet && HasWornArmorInSlot(target, Slot::kFeet)) {
            slots += 1.0F;
        }

        return slots * settings.hiddenArmorPerSlot;
    }

    [[nodiscard]] float GetStoredActorValue(RE::Actor& target, RE::ActorValue actorValue)
    {
        const auto& runtimeData = target.GetActorRuntimeData();

        auto result = 0.0F;
        auto found = false;

        if (const auto* base = runtimeData.avStorage.baseValues[actorValue]) {
            result += *base;
            found = true;
        }

        if (const auto* modifiers = runtimeData.avStorage.modifiers[actorValue]) {
            result += modifiers->modifiers[RE::ACTOR_VALUE_MODIFIER::kPermanent];
            result += modifiers->modifiers[RE::ACTOR_VALUE_MODIFIER::kTemporary];
            result += modifiers->modifiers[RE::ACTOR_VALUE_MODIFIER::kDamage];
            found = true;
        }

        if (!found && actorValue == RE::ActorValue::kDamageResist) {
            result = runtimeData.armorRating;
        }

        if (!std::isfinite(result)) {
            return 0.0F;
        }

        return result;
    }

    [[nodiscard]] RE::HitData* GetLastHitData(RE::Actor& target)
    {
        const auto* process = target.GetActorRuntimeData().currentProcess;
        if (!process || !process->middleHigh) {
            return nullptr;
        }

        return process->middleHigh->lastHitData;
    }

    [[nodiscard]] bool HitDataMatches(RE::Actor& target, RE::Actor* attacker, const RE::HitData& hitData)
    {
        if (const auto hitTarget = hitData.target.get(); hitTarget && hitTarget.get() != &target) {
            return false;
        }

        if (attacker) {
            if (const auto hitAggressor = hitData.aggressor.get(); hitAggressor && hitAggressor.get() != attacker) {
                return false;
            }
        }

        return true;
    }

    [[nodiscard]] float GetVanillaFinalPhysicalDamage(const RE::HitData& hitData, float incomingDamage)
    {
        const auto rawPhysical = (std::max)(0.0F, hitData.physicalDamage);
        if (rawPhysical <= 0.0F) {
            return 0.0F;
        }

        const auto resisted = std::clamp(hitData.resistedPhysicalDamage, 0.0F, rawPhysical);
        const auto finalPhysical = (std::max)(0.0F, rawPhysical - resisted);
        return (std::min)(finalPhysical, (std::max)(0.0F, incomingDamage));
    }

    [[nodiscard]] float FallbackRawPhysicalDamageFromVanillaResult(
        float incomingDamage,
        float armorRating,
        const MAF::Settings& settings)
    {
        const auto vanillaMult = (std::max)(0.01F, MAF::VanillaDamageMultiplier(armorRating, settings));
        return (std::max)(0.0F, incomingDamage) / vanillaMult;
    }

    [[nodiscard]] float AdjustUnidentifiedDamage(
        RE::Actor& target,
        float incomingDamage,
        float armorRating,
        float desiredMultiplier,
        const MAF::Settings& settings,
        std::string_view reason)
    {
        if (!settings.affectUnidentifiedHealthDamage) {
            if (settings.logAdjustments) {
                SKSE::log::info(
                    "Skipped unidentified damage: target={:08X}, reason={}, incoming={}",
                    target.GetFormID(),
                    reason,
                    incomingDamage);
            }
            return incomingDamage;
        }

        const auto desiredDamage = (std::max)(0.0F, incomingDamage) * desiredMultiplier;
        if (settings.logAdjustments) {
            SKSE::log::info(
                "Adjusted unidentified damage: target={:08X}, reason={}, armor={}, incoming={}, adjusted={}",
                target.GetFormID(),
                reason,
                armorRating,
                incomingDamage,
                desiredDamage);
        }

        return desiredDamage;
    }
}

namespace MAF
{
    float MorrowindDamageMultiplier(float armorRating, const Settings& settings)
    {
        const auto armor = (std::max)(0.0F, armorRating);
        const auto pct = (std::max)(0.0F, settings.percentHealthPerArmorPoint);
        const auto multiplier = 1.0F / (1.0F + armor * pct);
        return std::clamp(multiplier, settings.minDamageMultiplier, 1.0F);
    }

    float VanillaDamageMultiplier(float armorRating, const Settings& settings)
    {
        const auto armor = (std::max)(0.0F, armorRating);
        const auto reduction = (std::min)(settings.vanillaMaxReduction, armor * settings.vanillaArmorReductionPerPoint);
        return std::clamp(1.0F - reduction, 0.01F, 1.0F);
    }

    float GetArmorRating(RE::Actor& target, const Settings& settings)
    {
        auto armor = GetStoredActorValue(target, RE::ActorValue::kDamageResist);
        armor = (std::max)(0.0F, armor);

        if (settings.armorSource == ArmorSource::kDisplayedPlusHiddenArmorSlots) {
            armor += HiddenArmorBonus(target, settings);
        }

        return armor;
    }

    float AdjustHealthDamage(RE::Actor& target, RE::Actor* attacker, float incomingDamage, const Settings& settings)
    {
        if (incomingDamage <= 0.0F || !ShouldAffectActor(target, settings)) {
            return incomingDamage;
        }

        const auto armor = GetArmorRating(target, settings);
        const auto desiredMultiplier = MorrowindDamageMultiplier(armor, settings);

        auto* hitData = GetLastHitData(target);
        if (!hitData || !HitDataMatches(target, attacker, *hitData)) {
            if (settings.requireRecentHitData) {
                return AdjustUnidentifiedDamage(
                    target,
                    incomingDamage,
                    armor,
                    desiredMultiplier,
                    settings,
                    hitData ? std::string_view{ "last hit data mismatch" } :
                              std::string_view{ "missing last hit data" });
            }

            const auto rawPhysical = FallbackRawPhysicalDamageFromVanillaResult(incomingDamage, armor, settings);
            const auto desiredPhysical = rawPhysical * desiredMultiplier;
            return (std::max)(0.0F, desiredPhysical);
        }

        const auto rawPhysical = (std::max)(0.0F, hitData->physicalDamage);
        if (rawPhysical <= settings.staleHitDamageTolerance) {
            return AdjustUnidentifiedDamage(
                target,
                incomingDamage,
                armor,
                desiredMultiplier,
                settings,
                std::string_view{ "stale or zero physical hit data" });
        }

        const auto vanillaFinalPhysical = GetVanillaFinalPhysicalDamage(*hitData, incomingDamage);
        const auto desiredFinalPhysical = rawPhysical * desiredMultiplier;
        const auto adjusted = (std::max)(0.0F, incomingDamage - vanillaFinalPhysical + desiredFinalPhysical);

        if (settings.logAdjustments) {
            SKSE::log::info(
                "Adjusted hit damage: target={:08X}, armor={}, rawPhysical={}, vanillaPhysical={}, desiredPhysical={}, incoming={}, adjusted={}",
                target.GetFormID(),
                armor,
                rawPhysical,
                vanillaFinalPhysical,
                desiredFinalPhysical,
                incomingDamage,
                adjusted);
        }

        return adjusted;
    }
}
