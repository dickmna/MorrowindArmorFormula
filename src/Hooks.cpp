#include "Hooks.h"

#include "ArmorFormula.h"
#include "PCH.h"
#include "Settings.h"

namespace
{
    using HandleHealthDamage_t = void (*)(RE::Actor*, RE::Actor*, float);

    struct VTablePatch
    {
        const REL::VariantID* vtableID = nullptr;
        HandleHealthDamage_t originalHandleHealthDamage = nullptr;
        std::uintptr_t vtableAddress = 0;
        const char* name = "";
    };

    std::array<VTablePatch, 3> g_patches{ {
        { std::addressof(RE::VTABLE_Actor[0]), nullptr, 0, "Actor" },
        { std::addressof(RE::VTABLE_Character[0]), nullptr, 0, "Character" },
        { std::addressof(RE::VTABLE_PlayerCharacter[0]), nullptr, 0, "PlayerCharacter" },
    } };

    [[nodiscard]] std::uint32_t HandleHealthDamageIndex()
    {
        return REL::Relocate<std::uint32_t>(0x104, 0x104, 0x106);
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

        patch->originalHandleHealthDamage(target, attacker, adjustedDamage);
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

        SKSE::log::info("Patched {}::HandleHealthDamage vtable slot {}", patch.name, handleIndex);
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
