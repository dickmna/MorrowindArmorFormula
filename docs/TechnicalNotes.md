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
    - observedPhysicalDamage
    + morrowindStyleFinalPhysicalDamage * postArmorPhysicalMultiplier
```

`observedPhysicalDamage` is the smaller of Skyrim's armor-reduced physical damage and the incoming health damage that reached the hook. `postArmorPhysicalMultiplier` is inferred from those same values when the incoming health damage is lower than Skyrim's armor-reduced physical damage.

For vanilla Dragonhide, this preserves the independent `0.20x` physical damage multiplier without scanning `ArmorFFSelf100 [MGEF:000CDB75]` in the damage hook. Skyrim implements Dragonhide's 80% reduction through `DragonhideSpellPerk [PERK:00109639]` / `ModIncomingDamage`, not as a normal `DamageResist` armor value.

This keeps typed damage, enchantment damage, and other non-armor components intact while replacing the armor portion when reliable hit data is available.

The formula is applied only when the current `HandleHealthDamage` call has a non-null attacker and Skyrim's `lastHitData` has the same target and aggressor plus a positive physical component. Missing or mismatched hit data is passed through unchanged.

The plugin deliberately does not rewrite `Actor::CheckClampDamageModifier` health deltas. That generic actor-value path receives spells, poison, scripts, and other health damage without enough source information to classify it safely. Older `affectUnidentifiedHealthDamage`, `requireRecentHitData`, and vanilla fallback configuration keys are ignored.
