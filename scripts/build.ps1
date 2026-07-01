param(
    [ValidateSet("x64", "x86")]
    [string]$Platform = "x64",
    [switch]$SkipTests
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $root "build\$Platform"
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    throw "cl.exe was not found. Open 'x64 Native Tools Command Prompt for VS' or install Visual Studio Build Tools with the C++ desktop workload."
}

function Invoke-NativeCommand {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,
        [string[]]$Arguments = @()
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$FilePath failed with exit code $LASTEXITCODE"
    }
}

$common = @(
    "/nologo",
    "/std:c++17",
    "/EHsc",
    "/utf-8",
    "/DUNICODE",
    "/D_UNICODE",
    "/I$root\src",
    "/Fo$buildDir\"
)

$core = @("$root\src\core\GurungTransliterator.cpp")

Write-Host "Building transliterator tests..." -ForegroundColor Cyan
Invoke-NativeCommand cl.exe ($common + $core + @("$root\tests\transliterator_tests.cpp", "/Fe:$buildDir\gurung_tests.exe"))

if (-not $SkipTests) {
    Write-Host "Running transliterator tests..." -ForegroundColor Cyan
    Invoke-NativeCommand "$buildDir\gurung_tests.exe"
}

Write-Host "Building CLI..." -ForegroundColor Cyan
Invoke-NativeCommand cl.exe ($common + $core + @("$root\src\cli\gurung_translit_cli.cpp", "/Fe:$buildDir\gurung-translit.exe"))

Write-Host "Building TSF DLL..." -ForegroundColor Cyan
Invoke-NativeCommand cl.exe ($common + @("/LD") + $core + @(
    "$root\src\ime\DllMain.cpp",
    "$root\src\ime\EditSession.cpp",
    "$root\src\ime\TextService.cpp",
    "/Fe:$buildDir\GurungScientificIME_$Platform.dll",
    "/link",
    "/DEF:$root\src\ime\GurungScientificIME.def",
    "ole32.lib",
    "uuid.lib",
    "advapi32.lib",
    "user32.lib"
))

Write-Host "Building helper..." -ForegroundColor Cyan
Invoke-NativeCommand cl.exe ($common + @("$root\src\helper\GurungHelper.cpp", "/Fe:$buildDir\GurungHelper_$Platform.exe", "/link", "user32.lib", "shell32.lib"))

Write-Host "Build outputs are in $buildDir" -ForegroundColor Green
