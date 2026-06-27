param(
    [string]$Configuration = "Release",
    [string]$Version = "1.0.6",
    [string]$OutputDirectory = ""
)

$ErrorActionPreference = "Stop"

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path $root "dist"
}

$stage = Join-Path $root "dist\MorrowindArmorFormula-$Version"
$pluginDir = Join-Path $stage "Data\SKSE\Plugins"
$dll = Join-Path $root "build\vs2022-release\$Configuration\MorrowindArmorFormula.dll"
$pdb = Join-Path $root "build\vs2022-release\$Configuration\MorrowindArmorFormula.pdb"
$toml = Join-Path $root "package\Data\SKSE\Plugins\MorrowindArmorFormula.toml"

if (-not (Test-Path $dll)) {
    throw "DLL not found: $dll. Run scripts\build.ps1 first."
}

New-Item -ItemType Directory -Force -Path $pluginDir | Out-Null
Copy-Item -Force $dll $pluginDir
Copy-Item -Force $toml $pluginDir

if (Test-Path $pdb) {
    Copy-Item -Force $pdb $pluginDir
}

Copy-Item -Force (Join-Path $root "README.md") $stage
Copy-Item -Force (Join-Path $root "CHANGELOG.md") $stage

$zip = Join-Path $OutputDirectory "MorrowindArmorFormula-$Version.zip"
if (Test-Path $zip) {
    Remove-Item -LiteralPath $zip -Force
}

Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $zip -Force
Write-Host "Created $zip"
