# MorrowindArmorFormula

MorrowindArmorFormula replaces Skyrim's vanilla capped armor damage reduction with an effective-health armor formula inspired by the requested Morrowind-style armor behavior.

## What It Does

Skyrim normally treats each armor point as a fixed amount of damage reduction, then caps armor damage reduction at about 80%.

This plugin instead uses:

```text
positive armor multiplier = 1 / (1 + armorRating * percentHealthPerArmorPoint)
negative armor multiplier = 2 - (1 / (1 + abs(armorRating) * percentHealthPerArmorPoint))
```

Default examples:

```text
 -300 armor -> 175% physical damage
 -100 armor -> 150% physical damage
  100 armor -> 50% physical damage
  300 armor -> 25% physical damage
  900 armor -> 10% physical damage
```

Every point of positive armor grants the same effective-health value. Negative armor is handled as the odd-symmetric opposite: it increases physical damage by the same relative amount that equivalent positive armor would reduce it. The vanilla 80% cap is bypassed unless you configure your own cap.

Vanilla Dragonhide is preserved as a post-armor physical damage multiplier. Dragonhide is not a normal `DamageResist` armor bonus; the game implements its 80% reduction through a `ModIncomingDamage` perk multiplier. This plugin replaces only the armor-reduced physical component and keeps the multiplier already present in Skyrim's incoming hit damage.

## Requirements

- SKSE64 or SKSEVR
- Address Library for SKSE Plugins, or VR Address Library for SKSEVR

## Installation

Install with MO2/Vortex, or copy the archive contents into your Skyrim Data folder.

## Configuration

Edit:

```text
Data/SKSE/Plugins/MorrowindArmorFormula.toml
```

Important options:

```toml
percentHealthPerArmorPoint = 0.01
minDamageMultiplier = 0.0
armorSource = "displayed"
```

`minDamageMultiplier = 0.0` means no artificial maximum positive-armor reduction. Use `0.20` if you want an 80% cap on the new curve. Because negative armor is symmetric, `0.20` also caps negative-armor amplification at `180%`.

Version 1.0.7 removes the old active-effect Dragonhide check and preserves Dragonhide automatically for confirmed physical hits.

Version 1.0.8 removes unidentified damage scaling and the generic health actor-value hook. The formula now requires matching target/aggressor physical `HitData`, so spells, poison, scripts, traps, and other unconfirmed damage remain unchanged. Obsolete fallback settings in older configuration files are ignored.

## Compatibility Notes

The plugin adjusts confirmed physical hit damage through SKSE/CommonLibSSE. Mods that replace the same low-level health damage virtual function may conflict.
