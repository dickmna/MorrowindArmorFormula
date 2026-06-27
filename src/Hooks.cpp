#include "Hooks.h"

#include "ArmorFormula.h"
#include "PCH.h"
#include "Settings.h"

namespace
{
    using HandleHealthDamage_t = void (*)(RE::Actor*, RE::Actor*, float);
    using CheckClampDamageModifier_t = float (*)(RE::Actor*, RE::ActorValue, float);

    struct VTablePatch
    {
        const REL::VariantID* vtableID = nullptr;
        HandleHealthDamage_t originalHandleHealthDamage = nullptr;
        CheckClampDamageModifier_t originalCheckClampDamageModifier = nullptr;
        std::uintptr_t vtableAddress = 0;
        const char* name = "";
    };

    std::array<VTablePatch, 3> g_patches{ {
        { std::addressof(RE::VTABLE_Actor[0]), nullptr, nullptr, 0, "Actor" },
        { std::addressof(RE::VTABLE_Character[0]), nullptr, nullptr, 0, "Character" },
        { std::addressof(RE::VTABLE_PlayerCharacter[0]), nullptr, nullptr, 0, "PlayerCharacter" },
    } };

    [[nodiscard]] std::uint32_t HandleHealthDamageIndex()
    {
        return REL::Relocate<std::uint32_t>(0x104, 0x104, 0x106);
    }

    [[nodiscard]] std::uint32_t CheckClampDamageModifierIndex()
    {
        return REL::Relocate<std::uint32_t>(0x127, 0x127, 0x129);
    }

    [[nodiscard]] VTablePatch* FindPatch(RE::Actor* target)
    {
        if (!target) {
            return nullptr;
        }

        const auto vtable = *reinterpret_cast<std::uintptr_t*>(target);
        for (auto& patch : g_patches) {
            if (patch.vtableAddress == vtable) {
                return std::addressof(patch);
            }
        }

        return std::addressof(g_patches.front());
    }

    thread_local bool g_insideHandleHealthDamage = false;

    void HookedHandleHealthDamage(RE::Actor* target, RE::Actor* attacker, float damage)
    {
        const auto patch = FindPatch(target);
        if (!patch || !patch->originalHandleHealthDamage) {
            return;
        }

        auto adjustedDamage = damage;
        if (target) {
            adjustedDamage = MAF::AdjustHealthDamage(*target, attacker, damage, MAF::GetSettings());
            if (MAF::GetSettings().logAdjustments) {
                SKSE::log::info(
                    "HandleHealthDamage hook: target={:08X}, attacker={:08X}, incoming={}, outgoing={}",
                    target->GetFormID(),
                    attacker ? attacker->GetFormID() : 0,
                    damage,
                    adjustedDamage);
            }
        }

        const auto guard = g_insideHandleHealthDamage;
        g_insideHandleHealthDamage = true;
        patch->originalHandleHealthDamage(target, attacker, adjustedDamage);
        g_insideHandleHealthDamage = guard;
    }

    float HookedCheckClampDamageModifier(RE::Actor* target, RE::ActorValue actorValue, float delta)
    {
        const auto patch = FindPatch(target);
        if (!patch || !patch->originalCheckClampDamageModifier) {
            return delta;
        }

        auto adjustedDelta = delta;
        const auto& settings = MAF::GetSettings();
        if (!g_insideHandleHealthDamage &&
            target &&
            actorValue == RE::ActorValue::kHealth &&
            delta < 0.0F) {
            const auto incomingDamage = -delta;
            const auto adjustedDamage = MAF::AdjustHealthDamage(*target, nullptr, incomingDamage, settings);
            adjustedDelta = -adjustedDamage;
        }

        if (settings.logAdjustments && actorValue == RE::ActorValue::kHealth) {
            SKSE::log::info(
                "CheckClampDamageModifier hook: target={:08X}, delta={}, adjustedDelta={}, guarded={}",
                target ? target->GetFormID() : 0,
                delta,
                adjustedDelta,
                g_insideHandleHealthDamage);
        }

        return patch->originalCheckClampDamageModifier(target, actorValue, adjustedDelta);
    }

    void PatchVTable(VTablePatch& patch)
    {
        REL::Relocation<std::uintptr_t*> vtable{ *patch.vtableID };
        patch.vtableAddress = reinterpret_cast<std::uintptr_t>(vtable.get());

        const auto handleIndex = HandleHealthDamageIndex();
        auto* handleSlot = vtable.get() + handleIndex;
        patch.originalHandleHealthDamage = reinterpret_cast<HandleHealthDamage_t>(*handleSlot);
        const auto handleReplacement = reinterpret_cast<std::uintptr_t>(std::addressof(HookedHandleHealthDamage));
        REL::safe_write(reinterpret_cast<std::uintptr_t>(handleSlot), handleReplacement);

        const auto clampIndex = CheckClampDamageModifierIndex();
        auto* clampSlot = vtable.get() + clampIndex;
        patch.originalCheckClampDamageModifier = reinterpret_cast<CheckClampDamageModifier_t>(*clampSlot);
        const auto clampReplacement = reinterpret_cast<std::uintptr_t>(std::addressof(HookedCheckClampDamageModifier));
        REL::safe_write(reinterpret_cast<std::uintptr_t>(clampSlot), clampReplacement);

        SKSE::log::info(
            "Patched {} primary vtable: HandleHealthDamage slot {}, CheckClampDamageModifier slot {}",
            patch.name,
            handleIndex,
            clampIndex);
    }
}

namespace MAF::Hooks
{
    void Install()
    {
        for (auto& patch : g_patches) {
            PatchVTable(patch);
        }
    }
}
