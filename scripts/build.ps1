param(
    [string]$Configuration = "Release",
    [string]$CMakePrefixPath = "",
    [string]$VsDevCmd = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
)

$ErrorActionPreference = "Stop"

$root = Resolve-Path (Join-Path $PSScriptRoot "..")

function ConvertTo-CmdArg {
    param([string]$Value)

    if ($Value -match '[\s"]') {
        return '"' + ($Value -replace '"', '\"') + '"'
    }

    return $Value
}

function Invoke-CmdChecked {
    param([string]$Command)

    cmd /c "set PATH=& $Command"
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code $LASTEXITCODE`: $Command"
    }
}

Push-Location $root
try {
    if (-not $CMakePrefixPath -and $env:VCPKG_ROOT) {
        $CMakePrefixPath = @(
            Join-Path $env:VCPKG_ROOT "packages\commonlibsse-ng_x64-windows"
            Join-Path $env:VCPKG_ROOT "packages\fmt_x64-windows"
            Join-Path $env:VCPKG_ROOT "packages\rapidcsv_x64-windows"
            Join-Path $env:VCPKG_ROOT "packages\spdlog_x64-windows"
            Join-Path $env:VCPKG_ROOT "packages\xbyak_x64-windows"
        ) -join ";"
    }

    if (-not $CMakePrefixPath -and (Test-Path "C:\Users\19566\vcpkg\packages\commonlibsse-ng_x64-windows")) {
        $CMakePrefixPath = @(
            "C:\Users\19566\vcpkg\packages\commonlibsse-ng_x64-windows"
            "C:\Users\19566\vcpkg\packages\fmt_x64-windows"
            "C:\Users\19566\vcpkg\packages\rapidcsv_x64-windows"
            "C:\Users\19566\vcpkg\packages\spdlog_x64-windows"
            "C:\Users\19566\vcpkg\packages\xbyak_x64-windows"
        ) -join ";"
    }

    $configureArgs = @(
        "-S", ".",
        "-B", "build\vs2022-release",
        "-G", "Visual Studio 17 2022",
        "-A", "x64",
        "-DCMAKE_INSTALL_PREFIX=dist\MorrowindArmorFormula-1.0.4"
    )

    if ($CMakePrefixPath) {
        $configureArgs += "-DCMAKE_PREFIX_PATH=$CMakePrefixPath"
    }

    if (Test-Path $VsDevCmd) {
        $configure = "call `"$VsDevCmd`" -arch=x64 && cmake $(($configureArgs | ForEach-Object { ConvertTo-CmdArg $_ }) -join ' ')"
        $build = "call `"$VsDevCmd`" -arch=x64 && cmake --build build\vs2022-release --config $Configuration"
        $install = "call `"$VsDevCmd`" -arch=x64 && cmake --install build\vs2022-release --config $Configuration"

        Invoke-CmdChecked $configure
        Invoke-CmdChecked $build
        Invoke-CmdChecked $install
    } else {
        cmake @configureArgs
        if ($LASTEXITCODE -ne 0) { throw "CMake configure failed with exit code $LASTEXITCODE" }
        cmake --build "build\vs2022-release" --config $Configuration
        if ($LASTEXITCODE -ne 0) { throw "CMake build failed with exit code $LASTEXITCODE" }
        cmake --install "build\vs2022-release" --config $Configuration
        if ($LASTEXITCODE -ne 0) { throw "CMake install failed with exit code $LASTEXITCODE" }
    }
}
finally {
    Pop-Location
}
