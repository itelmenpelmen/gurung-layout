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
$gurungLanguageTag = "gvr-Deva-NP"
$gurungInputMethodTip = "1000:{FAEC539D-4867-4342-8E56-9953A9EF1701}{8E110837-E540-44AF-A8B4-48ED7D808B65}"
$gurungTipPattern = '^[0-9A-Fa-f]{4}:\{FAEC539D-4867-4342-8E56-9953A9EF1701\}\{8E110837-E540-44AF-A8B4-48ED7D808B65\}$'

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

function Set-GurungUserLanguageProfile {
    try {
        $languageList = Get-WinUserLanguageList
        $targetLanguage = $null

        foreach ($language in $languageList) {
            if ($language.LanguageTag -eq $gurungLanguageTag) {
                $targetLanguage = $language
            }

            $keptTips = New-Object System.Collections.Generic.List[string]
            foreach ($tip in $language.InputMethodTips) {
                if (-not [string]::IsNullOrWhiteSpace($tip) -and $tip -notmatch $gurungTipPattern) {
                    [void]$keptTips.Add($tip)
                }
            }

            $language.InputMethodTips.Clear()
            foreach ($tip in $keptTips) {
                [void]$language.InputMethodTips.Add($tip)
            }
        }

        if ($null -eq $targetLanguage) {
            $newLanguageList = New-WinUserLanguageList $gurungLanguageTag
            $targetLanguage = $newLanguageList[0]
            $targetLanguage.InputMethodTips.Clear()
            [void]$languageList.Add($targetLanguage)
        }

        if (-not $targetLanguage.InputMethodTips.Contains($gurungInputMethodTip)) {
            [void]$targetLanguage.InputMethodTips.Add($gurungInputMethodTip)
        }

        Set-WinUserLanguageList $languageList -Force
        Write-Host "Windows language profile added: Western Gurung (gvr-Deva-NP)." -ForegroundColor Green
    } catch {
        Write-Warning "Installed the IME, but could not update the Windows language list automatically: $($_.Exception.Message)"
        Write-Warning "If needed, add Western Gurung (gvr-Deva-NP) in Settings and choose Gurung Scientific IME."
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
Set-GurungUserLanguageProfile

Start-Process "ctfmon.exe"

if (Test-Path (Join-Path $installDir (Split-Path -Leaf $helper))) {
    Start-Process -FilePath (Join-Path $installDir (Split-Path -Leaf $helper)) -WindowStyle Hidden
}

Write-Host "Gurung Scientific IME installed under Western Gurung (gvr-Deva-NP)." -ForegroundColor Green
Write-Host "Registered DLL: $installedDll" -ForegroundColor Green
Write-Host "SHA256: $installedHash" -ForegroundColor Green
