# =============================================================================
# Riddick VR — Unified Multi-Arch Build Script (inspired by F.E.A.R. VR)
# =============================================================================
[CmdletBinding()]
param(
    [string]$Config = "Release",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir = Split-Path -Parent $ScriptDir

Write-Host "========================================================" -ForegroundColor Cyan
Write-Host "  Riddick VR: Building x64 Host & Win32 Hook Artifacts  " -ForegroundColor Cyan
Write-Host "========================================================" -ForegroundColor Cyan

# Locate CMake
$cmakePath = "cmake"
if (-not (Get-Command "cmake" -ErrorAction SilentlyContinue)) {
    $vsCMake = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    if (Test-Path $vsCMake) {
        $cmakePath = $vsCMake
    } else {
        $vsCMakeCommunity = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
        if (Test-Path $vsCMakeCommunity) {
            $cmakePath = $vsCMakeCommunity
        } else {
            throw "CMake not found! Please ensure CMake is installed or on PATH."
        }
    }
}
Write-Host "Using CMake: $cmakePath" -ForegroundColor DarkGray

$DistDir = Join-Path $RootDir "dist"
if (-not (Test-Path $DistDir)) {
    New-Item -ItemType Directory -Path $DistDir | Out-Null
}

# -----------------------------------------------------------------------------
# Step 1: Build x64 OpenXR Companion Host (riddickvr-host.exe)
# -----------------------------------------------------------------------------
Write-Host "`n>>> [1/2] Building x64 OpenXR Companion Host (riddickvr-host.exe)..." -ForegroundColor Yellow
$x64BuildDir = Join-Path $RootDir "build\x64"

if ($Clean -and (Test-Path $x64BuildDir)) {
    Remove-Item -Recurse -Force $x64BuildDir
}

& $cmakePath -S $RootDir -B $x64BuildDir -A x64 -DRIDDICKVR_BUILD_HOST=ON -DRIDDICKVR_BUILD_PROXY=OFF
& $cmakePath --build $x64BuildDir --config $Config

$hostBinary = Join-Path $x64BuildDir "$Config\riddickvr-host.exe"
if (Test-Path $hostBinary) {
    Copy-Item -Force $hostBinary $DistDir
    Write-Host "  -> Successfully packaged dist\riddickvr-host.exe (x64)" -ForegroundColor Green
} else {
    throw "Failed to find built x64 host binary at $hostBinary"
}

# -----------------------------------------------------------------------------
# Step 2: Build 32-bit Game Proxy (opengl32.dll)
# -----------------------------------------------------------------------------
Write-Host "`n>>> [2/2] Building 32-bit Game Proxy (opengl32.dll)..." -ForegroundColor Yellow
$x86BuildDir = Join-Path $RootDir "build\x86"

if ($Clean -and (Test-Path $x86BuildDir)) {
    Remove-Item -Recurse -Force $x86BuildDir
}

& $cmakePath -S $RootDir -B $x86BuildDir -A Win32 -DRIDDICKVR_BUILD_HOST=OFF -DRIDDICKVR_BUILD_PROXY=ON
& $cmakePath --build $x86BuildDir --config $Config

$hookBinary = Join-Path $x86BuildDir "$Config\opengl32.dll"
if (Test-Path $hookBinary) {
    Copy-Item -Force $hookBinary $DistDir
    Write-Host "  -> Successfully packaged dist\opengl32.dll (x86)" -ForegroundColor Green
} else {
    throw "Failed to find built 32-bit hook binary at $hookBinary"
}

# -----------------------------------------------------------------------------
# Summary & Quickstart instructions
# -----------------------------------------------------------------------------
$distReadme = @"
Riddick VR — Release Artifacts
=============================
1. Copy 'opengl32.dll' to your Dark Athena executable directory:
   e.g. <GameFolder>\System\Win32_x86\opengl32.dll

2. Launch 'riddickvr-host.exe' with your VR headset connected.
   (Run 'riddickvr-host.exe --validate-only' to check OpenXR runtime setup).

3. Launch Dark Athena! The game will link to the 64-bit host over shared memory.
"@
Set-Content -Path (Join-Path $DistDir "README.txt") -Value $distReadme

Write-Host "`n========================================================" -ForegroundColor Green
Write-Host "  BUILD COMPLETE: Artifacts packaged in 'dist/'         " -ForegroundColor Green
Write-Host "========================================================" -ForegroundColor Green
Get-ChildItem $DistDir | Select-Object Name, Length, LastWriteTime | Format-Table -AutoSize
