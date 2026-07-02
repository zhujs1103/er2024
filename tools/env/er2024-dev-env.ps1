param([switch]$Quiet)

$ErrorActionPreference = "Stop"

$script:EnvRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$script:WorkspaceRoot = Resolve-Path (Join-Path $script:EnvRoot "..\..")

$env:ER2024_WORKSPACE = $script:WorkspaceRoot.Path
$env:ER2024_KEIL_ROOT = "D:\Keil_v5"
$env:ER2024_SYSCONFIG_ROOT = "D:\ti\sysconfig_1.28.0"

$sdkCandidates = @(
    "D:\ti\mspm0_sdk_2_10_00_04",
    "C:\ti\mspm0_sdk_2_10_00_04"
)

$sdkRoot = $sdkCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if ($sdkRoot) {
    $env:MSPM0_SDK_ROOT = $sdkRoot
} elseif (-not $env:MSPM0_SDK_ROOT) {
    $env:MSPM0_SDK_ROOT = "D:\ti\mspm0_sdk_2_10_00_04"
}

$pathItems = @(
    (Join-Path $env:ER2024_KEIL_ROOT "UV4"),
    (Join-Path $env:ER2024_KEIL_ROOT "ARM\ARMCLANG\bin"),
    (Join-Path $env:ER2024_KEIL_ROOT "ARM\ARMCC\bin"),
    $env:ER2024_SYSCONFIG_ROOT
)

foreach ($item in $pathItems) {
    if ((Test-Path -LiteralPath $item) -and (($env:PATH -split ";") -notcontains $item)) {
        $env:PATH = "$item;$env:PATH"
    }
}

if (-not $Quiet) {
    Write-Host "ER2024 dev environment activated."
    Write-Host "Workspace:      $env:ER2024_WORKSPACE"
    Write-Host "Keil root:      $env:ER2024_KEIL_ROOT"
    Write-Host "SysConfig root: $env:ER2024_SYSCONFIG_ROOT"
    Write-Host "MSPM0 SDK root: $env:MSPM0_SDK_ROOT"
}
