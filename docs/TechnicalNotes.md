# Technical Notes

The plugin does not raise or spoof `DamageResist` to fit Skyrim's original formula. That approach would still hit Skyrim's vanilla armor cap.

Instead, it hooks `Actor::HandleHealthDamage`, checks the target actor's `lastHitData`, estimates the vanilla final physical component, and replaces that physical component with:

```text
rawPhysicalDamage / (1 + armorRating * percentHealthPerArmorPoint)
```

The rest of the incoming health damage is preserved:

```text
adjustedHealthDamage =
    incomingHealthDamage
    - vanillaFinalPhysicalDamage
    + morrowindStyleFinalPhysicalDamage
```

This keeps typed damage, enchantment damage, and other non-armor components intact while replacing the armor portion. With `requireRecentHitData = true`, the plugin refuses to adjust damage unless a recent physical hit can be identified.
