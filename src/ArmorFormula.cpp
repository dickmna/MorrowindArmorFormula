#include "ArmorFormula.h"

#include "PCH.h"

namespace
{
    constexpr RE::FormID kDragonhideEffectFormID = 0x000CDB75;

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

    [[nodiscard]] float SafeActorValueModifier(
        RE::Actor& target,
        RE::ACTOR_VALUE_MODIFIER modifier,
        RE::ActorValue actorValue)
    {
        const auto value = target.GetActorValueModifier(modifier, actorValue);
        if (!std::isfinite(value)) {
            return 0.0F;
        }

        return value;
    }

    [[nodiscard]] float GetNonVirtualModifierTotal(RE::Actor& target, RE::ActorValue actorValue)
    {
        auto total = 0.0F;
        total += SafeActorValueModifier(target, RE::ACTOR_VALUE_MODIFIER::kPermanent, actorValue);
        total += SafeActorValueModifier(target, RE::ACTOR_VALUE_MODIFIER::kTemporary, actorValue);
        total += SafeActorValueModifier(target, RE::ACTOR_VALUE_MODIFIER::kDamage, actorValue);
        return total;
    }

    [[nodiscard]] float FiniteOrZero(float value)
    {
        return std::isfinite(value) ? value : 0.0F;
    }

    [[nodiscard]] float GetStoredActorValue(RE::Actor& target, RE::ActorValue actorValue)
    {
        const auto& runtimeData = target.GetActorRuntimeData();

        auto baseValue = 0.0F;
        auto storedModifierTotal = 0.0F;
        auto foundBase = false;
        auto foundModifiers = false;

        if (const auto* base = runtimeData.avStorage.baseValues[actorValue]) {
            baseValue = FiniteOrZero(*base);
            foundBase = true;
        }

        if (const auto* modifiers = runtimeData.avStorage.modifiers[actorValue]) {
            storedModifierTotal += FiniteOrZero(modifiers->modifiers[RE::ACTOR_VALUE_MODIFIER::kPermanent]);
            storedModifierTotal += FiniteOrZero(modifiers->modifiers[RE::ACTOR_VALUE_MODIFIER::kTemporary]);
            storedModifierTotal += FiniteOrZero(modifiers->modifiers[RE::ACTOR_VALUE_MODIFIER::kDamage]);
            foundModifiers = true;
        }

        auto result = foundBase || foundModifiers ? baseValue + storedModifierTotal : 0.0F;
        if (actorValue != RE::ActorValue::kDamageResist) {
            return result;
        }

        const auto cachedArmor = FiniteOrZero(runtimeData.armorRating);
        if (!foundBase && !foundModifiers) {
            result = cachedArmor;
        }

        const auto baseComponent = std::abs(baseValue) >= std::abs(cachedArmor) ? baseValue : cachedArmor;
        const auto modifierTotal = GetNonVirtualModifierTotal(target, actorValue);
        const auto selectedModifierTotal =
            std::abs(modifierTotal) > 0.0001F || !foundModifiers ? modifierTotal : storedModifierTotal;
        result = baseComponent + selectedModifierTotal;

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

    [[nodiscard]] bool IsInactiveOrDispelled(const RE::ActiveEffect& activeEffect)
    {
        return activeEffect.flags.any(RE::ActiveEffect::Flag::kInactive, RE::ActiveEffect::Flag::kDispelled);
    }

    [[nodiscard]] bool HasActiveMagicEffect(RE::Actor& target, RE::FormID effectFormID)
    {
        auto* activeEffects = target.GetActiveEffectList();
        if (!activeEffects) {
            return false;
        }

        for (const auto* activeEffect : *activeEffects) {
            if (!activeEffect || IsInactiveOrDispelled(*activeEffect)) {
                continue;
            }

            const auto* effect = activeEffect->effect;
            const auto* baseEffect = effect ? effect->baseEffect : nullptr;
            if (baseEffect && baseEffect->GetFormID() == effectFormID) {
                return true;
            }
        }

        return false;
    }

    [[nodiscard]] float ExtraPhysicalDamageMultiplier(RE::Actor& target, const MAF::Settings& settings)
    {
        if (settings.preserveDragonhide && HasActiveMagicEffect(target, kDragonhideEffectFormID)) {
            return settings.dragonhideDamageMultiplier;
        }

        return 1.0F;
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
        if (!std::isfinite(armorRating)) {
            return 1.0F;
        }

        const auto armor = std::abs(armorRating);
        const auto pct = (std::max)(0.0F, settings.percentHealthPerArmorPoint);
        const auto defensiveMultiplier = std::clamp(
            1.0F / (1.0F + armor * pct),
            settings.minDamageMultiplier,
            1.0F);

        if (armorRating >= 0.0F) {
            return defensiveMultiplier;
        }

        return 1.0F + (1.0F - defensiveMultiplier);
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
        const auto extraPhysicalMultiplier = ExtraPhysicalDamageMultiplier(target, settings);

        const auto* player = RE::PlayerCharacter::GetSingleton();
        if (settings.logAdjustments && player && target.GetFormID() == player->GetFormID()) {
            SKSE::log::info(
                "Armor rating read: target={:08X}, armor={}, damageMultiplier={}, extraPhysicalMultiplier={}",
                target.GetFormID(),
                armor,
                desiredMultiplier,
                extraPhysicalMultiplier);
        }

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
        const auto desiredFinalPhysical = rawPhysical * desiredMultiplier * extraPhysicalMultiplier;
        const auto adjusted = (std::max)(0.0F, incomingDamage - vanillaFinalPhysical + desiredFinalPhysical);

        if (settings.logAdjustments) {
            SKSE::log::info(
                "Adjusted hit damage: target={:08X}, armor={}, extraPhysicalMultiplier={}, rawPhysical={}, vanillaPhysical={}, desiredPhysical={}, incoming={}, adjusted={}",
                target.GetFormID(),
                armor,
                extraPhysicalMultiplier,
                rawPhysical,
                vanillaFinalPhysical,
                desiredFinalPhysical,
                incomingDamage,
                adjusted);
        }

        return adjusted;
    }
}
