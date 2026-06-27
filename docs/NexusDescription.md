# MorrowindArmorFormula

MorrowindArmorFormula replaces Skyrim's vanilla capped armor damage reduction with an effective-health armor formula inspired by the requested Morrowind-style armor behavior.

## What It Does

Skyrim normally treats each armor point as a fixed amount of damage reduction, then caps armor damage reduction at about 80%.

This plugin instead uses:

```text
physical damage multiplier = 1 / (1 + armorRating * percentHealthPerArmorPoint)
```

Default examples:

```text
100 armor -> 50% physical damage
300 armor -> 25% physical damage
900 armor -> 10% physical damage
```

Every point of armor grants the same effective-health value. The vanilla 80% cap is bypassed unless you configure your own cap.

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

`minDamageMultiplier = 0.0` means no artificial maximum reduction. Use `0.20` if you want an 80% cap on the new curve.

## Compatibility Notes

The plugin adjusts recent physical hit damage through SKSE/CommonLibSSE. Magic damage and script damage are not changed by default. Mods that replace the same low-level health damage virtual function may conflict.
