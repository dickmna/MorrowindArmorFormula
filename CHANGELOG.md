# Changelog

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
