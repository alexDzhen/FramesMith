# Builds framesmith in release mode and runs the tests.
# Run from "Developer PowerShell for VS", where cmake and the compiler are on the PATH.
$ErrorActionPreference = "Stop"

Set-Location (Join-Path $PSScriptRoot "..")

function Invoke-Step {
    param([string]$Command, [string[]]$Arguments)
    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

Invoke-Step cmake @("-S", ".", "-B", "build", "-DCMAKE_BUILD_TYPE=Release")
Invoke-Step cmake @("--build", "build", "--config", "Release", "--parallel")
Invoke-Step ctest @("--test-dir", "build", "-C", "Release", "--output-on-failure")

$binary = Get-ChildItem -Path build -Recurse -Filter framesmith.exe | Select-Object -First 1
Write-Host ""
Write-Host "Done. Build a frame with: $($binary.FullName) --dst broadcast --type arp --payload `"00 01`""
