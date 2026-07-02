param(
    [ValidateSet("x64", "x86")]
    [string]$Platform = "x64"
)

$ErrorActionPreference = "Stop"

if (-not ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Start-Process powershell.exe "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`" -Platform $Platform" -Verb RunAs -Wait
    exit
}

$installDir = Join-Path $env:ProgramFiles "Berkeley Computer\GurungScientificIME"
$gurungLanguageTag = "gvr-Deva-NP"
$gurungTipPattern = '^[0-9A-Fa-f]{4}:\{FAEC539D-4867-4342-8E56-9953A9EF1701\}\{8E110837-E540-44AF-A8B4-48ED7D808B65\}$'
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

function Remove-GurungUserLanguageProfile {
    try {
        $languageList = Get-WinUserLanguageList

        for ($index = $languageList.Count - 1; $index -ge 0; --$index) {
            $language = $languageList[$index]
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

            if ($language.LanguageTag -eq $gurungLanguageTag -and $language.InputMethodTips.Count -eq 0) {
                $languageList.RemoveAt($index)
            }
        }

        Set-WinUserLanguageList $languageList -Force
    } catch {
        Write-Warning "Could not update the Windows language list automatically: $($_.Exception.Message)"
    }
}

Stop-Process -Name "GurungHelper_x64", "GurungHelper_x86" -Force -ErrorAction SilentlyContinue
Stop-Process -Name "ctfmon" -Force -ErrorAction SilentlyContinue

if (Test-Path $installDir) {
    Get-ChildItem -Path $installDir -Filter "GurungScientificIME_$Platform*.dll" -ErrorAction SilentlyContinue | ForEach-Object {
        Invoke-NativeCommand $regsvr32 @("/u", "/s", $_.FullName)
    }
}

Remove-GurungUserLanguageProfile

Start-Sleep -Seconds 1
Remove-Item -Recurse -Force -Path $installDir -ErrorAction SilentlyContinue
Start-Process "ctfmon.exe"

Write-Host "Gurung Scientific IME uninstalled." -ForegroundColor Green
