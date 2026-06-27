# Changelog

## 1.0.5

- Adds odd-symmetric negative armor scaling: negative armor increases physical damage by the same relative amount that positive armor would reduce it.
- Keeps `minDamageMultiplier` symmetrical; for example, `0.20` caps positive reduction at `0.20x` and negative amplification at `1.80x`.
- Improves signed `DamageResist` reconstruction by combining cached/base armor with actor-value modifiers instead of discarding negative totals.

## 1.0.4

- Includes non-virtual `DamageResist` actor-value modifiers when calculating armor.
- Fixes console `setav/forceav damageresist` test values being ignored.
- Adds explicit player armor/multiplier logging for verification.

## 1.0.3

- Fixes a crash in armor reading by avoiding `ActorValueOwner::GetActorValue`.
- Reads `DamageResist` directly from actor value storage, with `armorRating` as a fallback.
- Avoids `GetActorBase` during affect-target checks.

## 1.0.2

- Adds a second hook at `Actor::CheckClampDamageModifier` for final health actor-value damage.
- Prevents double scaling when both the health-damage hook and actor-value hook are reached.
- Logs health actor-value clamp calls for in-game verification.

## 1.0.1

- Adds fallback scaling for unidentified health damage when recent hit data is unavailable.
- Enables adjustment logging by default to make in-game verification easier.
- Logs loaded settings and each health-damage adjustment path.

## 1.0.0

- Initial release.
- Replaces vanilla capped armor damage reduction with effective-health armor scaling.
- Adds configurable armor-to-EHP percentage.
- Adds optional hidden armor-slot inclusion.
- Leaves non-physical health damage unchanged by default.
