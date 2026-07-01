param(
    [ValidateSet("x64", "x86")]
    [string]$Platform = "x64"
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $root "build\$Platform"
$packageDir = Join-Path $root "release\GurungScientificIME_Release_$Platform"
New-Item -ItemType Directory -Force -Path $packageDir | Out-Null

Copy-Item (Join-Path $buildDir "GurungScientificIME_$Platform.dll") $packageDir -Force
Copy-Item (Join-Path $buildDir "GurungHelper_$Platform.exe") $packageDir -Force -ErrorAction SilentlyContinue
Copy-Item (Join-Path $root "scripts\install.ps1") $packageDir -Force
Copy-Item (Join-Path $root "scripts\uninstall.ps1") $packageDir -Force
Copy-Item (Join-Path $root "docs\typing-guide.md") $packageDir -Force
Copy-Item (Join-Path $root "README.md") $packageDir -Force
Copy-Item (Join-Path $root "data\gurung_inventory.yaml") $packageDir -Force

Write-Host "Release package staged in $packageDir" -ForegroundColor Green
