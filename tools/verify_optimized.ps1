param(
    [string]$Compiler = 'g++',
    [string]$BuildDirectory = '.dist/milestone3',
    [string]$HistoricalRoot = '',
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug',
    [switch]$Benchmarks
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$projectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if (-not [System.IO.Path]::IsPathRooted($BuildDirectory)) { $BuildDirectory = Join-Path $projectRoot $BuildDirectory }
$BuildDirectory = [System.IO.Path]::GetFullPath($BuildDirectory)
# The reference verifier validates the output path against historical roots and .git first.
& (Join-Path $PSScriptRoot 'verify_reference.ps1') -Compiler $Compiler -BuildDirectory $BuildDirectory -HistoricalRoot $HistoricalRoot -Configuration $Configuration
if (-not $?) { throw 'Reference verification failed' }
function Invoke-Checked {
    param([string]$File, [string[]]$Arguments)
    & $File @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$File failed with exit code $LASTEXITCODE" }
}
$flags = @('-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Wconversion', '-Wshadow', '-Werror')
if ($Configuration -eq 'Debug') { $flags += @('-O0', '-g', '-D_GLIBCXX_ASSERTIONS') }
else { $flags += @('-O2', '-DNDEBUG') }
$includeFlags = @('-I', (Join-Path $projectRoot 'include'))
$objectPath = Join-Path $BuildDirectory "optimized_$Configuration.o"
Invoke-Checked $Compiler ($flags + $includeFlags + @('-c', (Join-Path $projectRoot 'src/optimized.cpp'), '-o', $objectPath))
$objects = @((Join-Path $BuildDirectory "reference_$Configuration.o"), $objectPath)
foreach ($target in @('optimized_tests', 'optimized_fixture_driver')) {
    $binaryPath = Join-Path $BuildDirectory "${target}_$Configuration.exe"
    Invoke-Checked $Compiler ($flags + $includeFlags + @((Join-Path $projectRoot "tests/$target.cpp")) + $objects + @('-o', $binaryPath))
    if ($target -eq 'optimized_tests') { Invoke-Checked $binaryPath @() }
}
Invoke-Checked 'python' @((Join-Path $projectRoot 'tools/verify_optimized.py'), '--driver', (Join-Path $BuildDirectory "optimized_fixture_driver_$Configuration.exe"))
if ($Benchmarks) {
    if ($Configuration -ne 'Release') { throw 'Benchmarks require Release' }
    $benchmark = Join-Path $BuildDirectory 'search_benchmark.exe'
    Invoke-Checked $Compiler ($flags + $includeFlags + @((Join-Path $projectRoot 'benchmarks/search_benchmark.cpp')) + $objects + @('-lpsapi', '-o', $benchmark))
    Invoke-Checked 'python' @((Join-Path $projectRoot 'tools/benchmark_search.py'), '--driver', $benchmark, '--output', (Join-Path $BuildDirectory 'benchmark-results.json'), '--compiler', $Compiler)
}
Write-Output "PASS: complete available Milestone 3 $Configuration verification suite"
