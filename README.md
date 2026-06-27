# MorrowindArmorFormula

MorrowindArmorFormula is an SKSE plugin that replaces Skyrim's capped linear armor damage reduction with an effective-health style armor formula.

```text
positive armor multiplier = 1 / (1 + armorRating * percentHealthPerArmorPoint)
negative armor multiplier = 2 - (1 / (1 + abs(armorRating) * percentHealthPerArmorPoint))
```

With the default `percentHealthPerArmorPoint = 0.01`:

```text
 -300 armor -> 175% physical damage
 -100 armor -> 150% physical damage
  100 armor -> 50% physical damage
  300 armor -> 25% physical damage
  900 armor -> 10% physical damage
```

This bypasses Skyrim's vanilla 80% armor damage reduction cap because it adjusts final health damage after vanilla physical armor reduction has already been calculated.

## Requirements

- Skyrim Special Edition, Anniversary Edition, or VR
- SKSE64 / SKSEVR matching the runtime
- Address Library for SKSE Plugins, or VR Address Library for SKSEVR

## Installation

Install with a mod manager, or copy the package contents into the Skyrim Data folder:

```text
Data/SKSE/Plugins/MorrowindArmorFormula.dll
Data/SKSE/Plugins/MorrowindArmorFormula.toml
```

## Configuration

Edit:

```text
Data/SKSE/Plugins/MorrowindArmorFormula.toml
```

The main setting is:

```toml
percentHealthPerArmorPoint = 0.01
```

Set `minDamageMultiplier = 0.0` for no artificial positive-armor cap. Set it to `0.20` to restore an 80% maximum reduction while keeping the new curve. Negative armor uses the same curve in reverse, so `minDamageMultiplier = 0.20` also caps negative-armor amplification at `180%`.

Vanilla Dragonhide is preserved separately. It is not treated as armor; when the vanilla `ArmorFFSelf100` effect is active on a confirmed physical hit, the plugin applies Dragonhide's `0.20x` physical damage multiplier after the Morrowind-style armor formula.

For testing and compatibility, `affectUnidentifiedHealthDamage = true` also scales health damage when Skyrim does not expose recent physical hit data to the plugin. Set it to `false` if you only want confirmed physical hits to be adjusted.

`logAdjustments = true` writes adjustment details to `Documents/My Games/Skyrim Special Edition/SKSE/MorrowindArmorFormula.log`.

## Build

Install Visual Studio 2022 Build Tools and vcpkg, then run:

```powershell
.\scripts\build.ps1
.\scripts\package.ps1
```

If `commonlibsse-ng` is installed in vcpkg but not discoverable, pass a prefix:

```powershell
.\scripts\build.ps1 -CMakePrefixPath "C:\path\to\vcpkg\packages\commonlibsse-ng_x64-windows;C:\path\to\vcpkg\packages\fmt_x64-windows;C:\path\to\vcpkg\packages\spdlog_x64-windows"
```

Some CommonLibSSE-NG installations also require adding the `rapidcsv_x64-windows` and `xbyak_x64-windows` package paths to the same prefix list.

## Compatibility

The plugin hooks `Actor::HandleHealthDamage` and uses `lastHitData` to adjust recent physical hits when available. If `affectUnidentifiedHealthDamage` is enabled, unidentified health damage is also scaled by the armor formula as a fallback.

The plugin explicitly preserves vanilla Dragonhide (`ArmorFFSelf100 [MGEF:000CDB75]`) on confirmed physical hits by applying its separate `0.20x` incoming-damage multiplier after the armor formula.

Other SKSE plugins that also replace `Actor::HandleHealthDamage` may conflict depending on load order and hook implementation.

## License

MIT
