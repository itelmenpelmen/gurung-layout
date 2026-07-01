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

Stop-Process -Name "GurungHelper_x64", "GurungHelper_x86" -Force -ErrorAction SilentlyContinue
Stop-Process -Name "ctfmon" -Force -ErrorAction SilentlyContinue

if (Test-Path $installDir) {
    Get-ChildItem -Path $installDir -Filter "GurungScientificIME_$Platform*.dll" -ErrorAction SilentlyContinue | ForEach-Object {
        Invoke-NativeCommand $regsvr32 @("/u", "/s", $_.FullName)
    }
}

Start-Sleep -Seconds 1
Remove-Item -Recurse -Force -Path $installDir -ErrorAction SilentlyContinue
Start-Process "ctfmon.exe"

Write-Host "Gurung Scientific IME uninstalled." -ForegroundColor Green
