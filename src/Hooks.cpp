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
        HandleHealthDamage_t original = nullptr;
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

    [[nodiscard]] HandleHealthDamage_t FindOriginal(RE::Actor* target)
    {
        if (!target) {
            return nullptr;
        }

        const auto vtable = *reinterpret_cast<std::uintptr_t*>(target);
        for (const auto& patch : g_patches) {
            if (patch.vtableAddress == vtable) {
                return patch.original;
            }
        }

        return g_patches.front().original;
    }

    void HookedHandleHealthDamage(RE::Actor* target, RE::Actor* attacker, float damage)
    {
        const auto original = FindOriginal(target);
        if (!original) {
            return;
        }

        auto adjustedDamage = damage;
        if (target) {
            adjustedDamage = MAF::AdjustHealthDamage(*target, attacker, damage, MAF::GetSettings());
        }

        original(target, attacker, adjustedDamage);
    }

    void PatchVTable(VTablePatch& patch)
    {
        REL::Relocation<std::uintptr_t*> vtable{ *patch.vtableID };
        const auto index = HandleHealthDamageIndex();
        auto* slot = vtable.get() + index;

        patch.vtableAddress = reinterpret_cast<std::uintptr_t>(vtable.get());
        patch.original = reinterpret_cast<HandleHealthDamage_t>(*slot);

        const auto replacement = reinterpret_cast<std::uintptr_t>(std::addressof(HookedHandleHealthDamage));
        REL::safe_write(reinterpret_cast<std::uintptr_t>(slot), replacement);

        SKSE::log::info("Patched {}::HandleHealthDamage vtable slot {}", patch.name, index);
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
