# Technical Notes

The plugin does not raise or spoof `DamageResist` to fit Skyrim's original formula. That approach would still hit Skyrim's vanilla armor cap.

Instead, it hooks `Actor::HandleHealthDamage`, checks the target actor's `lastHitData`, estimates the vanilla final physical component, and replaces that physical component with:

```text
if armorRating >= 0:
    rawPhysicalDamage / (1 + armorRating * percentHealthPerArmorPoint)

if armorRating < 0:
    rawPhysicalDamage * (2 - (1 / (1 + abs(armorRating) * percentHealthPerArmorPoint)))
```

Negative armor is an odd-symmetric extension around a `1.0x` damage multiplier: `+100` armor reduces default physical damage to `0.5x`, while `-100` armor increases it to `1.5x`.

The rest of the incoming health damage is preserved:

```text
adjustedHealthDamage =
    incomingHealthDamage
    - vanillaFinalPhysicalDamage
    + morrowindStyleFinalPhysicalDamage * extraPhysicalMultiplier
```

For vanilla Dragonhide, `extraPhysicalMultiplier` is `0.20` when `ArmorFFSelf100 [MGEF:000CDB75]` is active on the target. Dragonhide is preserved as an independent physical damage multiplier because Skyrim implements its 80% reduction through `DragonhideSpellPerk [PERK:00109639]` / `ModIncomingDamage`, not as a normal `DamageResist` armor value.

This keeps typed damage, enchantment damage, and other non-armor components intact while replacing the armor portion when reliable hit data is available.

If Skyrim does not expose recent hit data for a health-damage call, `affectUnidentifiedHealthDamage = true` scales the incoming health damage directly:

```text
adjustedHealthDamage =
    incomingHealthDamage
    * morrowindStyleDamageMultiplier
```

This fallback is useful for testing, console-driven damage, and compatibility with damage sources that bypass normal physical hit data. Set it to `false` if the plugin should only affect confirmed physical hits.
