$ErrorActionPreference = "Continue"

. (Join-Path $PSScriptRoot "er2024-dev-env.ps1") -Quiet

function Test-Tool {
    param(
        [Parameter(Mandatory=$true)][string]$Name,
        [Parameter(Mandatory=$true)][string]$Path,
        [string[]]$VersionArgs = @()
    )

    $exists = Test-Path -LiteralPath $Path
    if (-not $exists) {
        [pscustomobject]@{
            Item = $Name
            Status = "missing"
            Detail = $Path
        }
        return
    }

    $detail = $Path
    if ($VersionArgs.Count -gt 0) {
        try {
            $version = & $Path @VersionArgs 2>&1 | Select-Object -First 2
            $detail = ($version -join " ").Trim()
        } catch {
            $detail = "exists, version check failed: $($_.Exception.Message)"
        }
    }

    [pscustomobject]@{
        Item = $Name
        Status = "ok"
        Detail = $detail
    }
}

$checks = @()
$checks += Test-Tool "Keil uVision" (Join-Path $env:ER2024_KEIL_ROOT "UV4\UV4.exe")
$checks += Test-Tool "Arm Compiler 6" (Join-Path $env:ER2024_KEIL_ROOT "ARM\ARMCLANG\bin\armclang.exe") @("--version")
$checks += Test-Tool "Arm Compiler 5" (Join-Path $env:ER2024_KEIL_ROOT "ARM\ARMCC\bin\armcc.exe") @("--vsn")
$checks += Test-Tool "TI SysConfig" (Join-Path $env:ER2024_SYSCONFIG_ROOT "sysconfig_cli.bat") @("--version")
$checks += Test-Tool "MSPM0 SDK" (Join-Path $env:MSPM0_SDK_ROOT ".metadata\product.json")
$checks += Test-Tool "MSPM0 Keil DFP" "D:\Users\steven\AppData\Local\Arm\Packs\TexasInstruments\MSPM0G1X0X_G3X0X_DFP\1.3.1\TexasInstruments.MSPM0G1X0X_G3X0X_DFP.pdsc"

$checks | Format-Table -AutoSize

$missingCount = @($checks | Where-Object { $_.Status -eq "missing" }).Count
if ($missingCount -gt 0) {
    Write-Host ""
    Write-Host "Missing items remain. See project_management/development_environment.md."
    exit 1
}

exit 0
