# Builds the players' version and packs it into a zip for GitHub Releases.
#
#   powershell -ExecutionPolicy Bypass -File tools\makeRelease.ps1 -Version 1.0
#
# - Its own build folder (TatouSource\build\release), apart from the working one.
# - CLASSIC_REBORN_DEV_TOOLS=OFF: no item window (F9) nor object editor (F10);
#   the visual fixes of 3D objects are still included.
# - Release configuration: optimised, no debug data.
# - The zip holds only Launcher.exe, Tatou.exe, README.md, LICENSE and the
#   menu font's license (Gelasio-OFL.txt; the font is inside the programs): no
#   launcher.cfg (debug mode stays off), no logs, no UserData, no .pdb.
# The result goes to release\ClassicReborn-<version>-windows-x64.zip.

param(
    [Parameter(Mandatory = $true)][string]$Version
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$source = Join-Path $repo 'TatouSource'
$build = Join-Path $source 'build\release'
$output = Join-Path $build 'Fitd\Release'
$releaseFolder = Join-Path $repo 'release'
$name = "ClassicReborn-$Version-windows-x64"

Write-Host "== Configuring ($build)"
cmake -G "Visual Studio 17 2022" -A x64 -S $source -B $build -DCLASSIC_REBORN_DEV_TOOLS=OFF | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed' }

Write-Host '== Building Release'
cmake --build $build --config Release --target Launcher Fitd -- /m:2 /nodeReuse:false /v:minimal
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }

# The development tools must not be in the executable
Write-Host '== Checking that F9 and F10 are not included'
$exeText = [System.Text.Encoding]::ASCII.GetString([System.IO.File]::ReadAllBytes((Join-Path $output 'Tatou.exe')))
foreach ($marker in @('(prueba, F9)', '(prueba, F10)')) {
    if ($exeText.Contains($marker)) { throw "Tatou.exe still contains the development tools ($marker)" }
}

Write-Host '== Packing'
$staging = Join-Path $releaseFolder $name
if (Test-Path $staging) { Remove-Item $staging -Recurse -Force }
New-Item -ItemType Directory -Force $staging | Out-Null
Copy-Item (Join-Path $output 'Launcher.exe'), (Join-Path $output 'Tatou.exe') $staging
Copy-Item (Join-Path $repo 'README.md'), (Join-Path $repo 'LICENSE') $staging
Copy-Item (Join-Path $source 'Shared\assets\fonts\OFL.txt') (Join-Path $staging 'Gelasio-OFL.txt')

$zip = Join-Path $releaseFolder "$name.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $staging '*') -DestinationPath $zip
Remove-Item $staging -Recurse -Force

$sizeMb = [math]::Round((Get-Item $zip).Length / 1MB, 1)
Write-Host "== Done: $zip ($sizeMb MB)"
