param(
    [string]$Compiler = 'g++',
    [string]$BuildDirectory = '.dist/milestone4',
    [string]$HistoricalRoot = '',
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$projectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if ([string]::IsNullOrEmpty($HistoricalRoot)) { $HistoricalRoot = Join-Path $projectRoot 'historical' }
if (-not [System.IO.Path]::IsPathRooted($BuildDirectory)) { $BuildDirectory = Join-Path $projectRoot $BuildDirectory }
function Invoke-Checked {
    param([string]$File, [string[]]$Arguments)
    & $File @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$File failed with exit code $LASTEXITCODE" }
}
& (Join-Path $PSScriptRoot 'verify_optimized.ps1') -Compiler $Compiler -BuildDirectory $BuildDirectory -HistoricalRoot $HistoricalRoot -Configuration $Configuration
if (-not $?) { throw 'Milestone 1-3 regression verification failed' }
Invoke-Checked 'python' @((Join-Path $PSScriptRoot 'reconstruct_historical_metadata.py'), '--check', '--historical-root', $HistoricalRoot)
& (Join-Path $PSScriptRoot 'build_cli.ps1') -Compiler $Compiler -BuildDirectory $BuildDirectory -Configuration $Configuration -ApplicationTests
if (-not $?) { throw 'Application build/checks failed' }
Invoke-Checked 'python' @((Join-Path $PSScriptRoot 'verify_cli.py'), '--app', (Join-Path $BuildDirectory "tradeup_$Configuration.exe"), '--historical-root', $HistoricalRoot)
Write-Output "PASS: complete available Milestone 4 $Configuration verification suite"
