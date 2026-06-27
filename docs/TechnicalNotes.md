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

This keeps typed damage, enchantment damage, and other non-armor components intact while replacing the armor portion when reliable hit data is available.

If Skyrim does not expose recent hit data for a health-damage call, `affectUnidentifiedHealthDamage = true` scales the incoming health damage directly:

```text
adjustedHealthDamage =
    incomingHealthDamage
    / (1 + armorRating * percentHealthPerArmorPoint)
```

This fallback is useful for testing, console-driven damage, and compatibility with damage sources that bypass normal physical hit data. Set it to `false` if the plugin should only affect confirmed physical hits.
