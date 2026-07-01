param(
    [ValidateSet("x64", "x86")]
    [string]$Platform = "x64"
)

$ErrorActionPreference = "Stop"

if (-not ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Start-Process powershell.exe "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`" -Platform $Platform" -Verb RunAs -Wait
    exit
}

$root = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $root "build\$Platform"
$installDir = Join-Path $env:ProgramFiles "Berkeley Computer\GurungScientificIME"
$dll = Join-Path $buildDir "GurungScientificIME_$Platform.dll"
$helper = Join-Path $buildDir "GurungHelper_$Platform.exe"
$inventory = Join-Path $root "data\gurung_inventory.yaml"
$typingGuide = Join-Path $root "docs\typing-guide.md"

if (-not (Test-Path $dll)) {
    $packageDll = Join-Path $PSScriptRoot "GurungScientificIME_$Platform.dll"
    if (Test-Path $packageDll) {
        $dll = $packageDll
        $helper = Join-Path $PSScriptRoot "GurungHelper_$Platform.exe"
        $inventory = Join-Path $PSScriptRoot "gurung_inventory.yaml"
        $typingGuide = Join-Path $PSScriptRoot "typing-guide.md"
    }
}

$regsvr32 = if ($Platform -eq "x86") {
    Join-Path $env:windir "SysWOW64\regsvr32.exe"
} else {
    Join-Path $env:windir "System32\regsvr32.exe"
}

function Invoke-NativeCommand {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,
        [string[]]$Arguments = @()
    )

    $argumentLine = ($Arguments | ForEach-Object {
        if ($_ -match '[\s"]') {
            '"' + ($_ -replace '"', '\"') + '"'
        } else {
            $_
        }
    }) -join ' '

    $process = Start-Process -FilePath $FilePath -ArgumentList $argumentLine -Wait -PassThru -WindowStyle Hidden
    if ($process.ExitCode -ne 0) {
        throw "$FilePath failed with exit code $($process.ExitCode)"
    }
}

if (-not (Test-Path $dll)) {
    throw "Missing GurungScientificIME_$Platform.dll. Run scripts\build.ps1 first, or run this installer from a release package folder."
}

New-Item -ItemType Directory -Force -Path $installDir | Out-Null

Stop-Process -Name "GurungHelper_x64", "GurungHelper_x86" -Force -ErrorAction SilentlyContinue
Stop-Process -Name "ctfmon" -Force -ErrorAction SilentlyContinue

$sourceHash = (Get-FileHash $dll -Algorithm SHA256).Hash
$hashPrefix = $sourceHash.Substring(0, 12)
$installedDll = Join-Path $installDir "GurungScientificIME_${Platform}_$hashPrefix.dll"

Copy-Item -Path $dll -Destination $installedDll -Force
$installedHash = (Get-FileHash $installedDll -Algorithm SHA256).Hash
if ($sourceHash -ne $installedHash) {
    throw "Installed DLL hash mismatch. Source: $sourceHash Installed: $installedHash"
}

if (Test-Path $helper) {
    Copy-Item -Path $helper -Destination (Join-Path $installDir (Split-Path -Leaf $helper)) -Force
}
if (Test-Path $inventory) {
    Copy-Item -Path $inventory -Destination $installDir -Force
}
if (Test-Path $typingGuide) {
    Copy-Item -Path $typingGuide -Destination $installDir -Force
}

Invoke-NativeCommand $regsvr32 @("/s", $installedDll)

Start-Process "ctfmon.exe"

if (Test-Path (Join-Path $installDir (Split-Path -Leaf $helper))) {
    Start-Process -FilePath (Join-Path $installDir (Split-Path -Leaf $helper)) -WindowStyle Hidden
}

Write-Host "Gurung Scientific IME installed. Add it under Settings > Time & Language > Language & Region > Nepali > Keyboards." -ForegroundColor Green
Write-Host "Registered DLL: $installedDll" -ForegroundColor Green
Write-Host "SHA256: $installedHash" -ForegroundColor Green
