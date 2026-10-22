param(
    [string]$Compiler = 'g++',
    [string]$BuildDirectory = 'build/reference-direct',
    [string]$HistoricalRoot = '',
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$projectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if ([string]::IsNullOrEmpty($HistoricalRoot)) { $HistoricalRoot = Join-Path $projectRoot 'historical' }
if (-not [System.IO.Path]::IsPathRooted($BuildDirectory)) { $BuildDirectory = Join-Path $projectRoot $BuildDirectory }
$BuildDirectory = [System.IO.Path]::GetFullPath($BuildDirectory)
foreach ($protectedPath in @((Join-Path $projectRoot 'historical'), (Join-Path $projectRoot '.git'), $HistoricalRoot)) {
    $protectedAbsolute = [System.IO.Path]::GetFullPath($protectedPath).TrimEnd('\', '/')
    if ($BuildDirectory.Equals($protectedAbsolute, [StringComparison]::OrdinalIgnoreCase) -or
        $BuildDirectory.StartsWith($protectedAbsolute + [System.IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Build output must be outside historical materials and .git.'
    }
}
New-Item -ItemType Directory -Force -Path $BuildDirectory | Out-Null
function Invoke-Checked {
    param([string]$File, [string[]]$Arguments)
    & $File @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$File failed with exit code $LASTEXITCODE" }
}
Invoke-Checked $Compiler @('--version')
$flags = @('-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Wconversion', '-Wshadow', '-Werror')
if ($Configuration -eq 'Debug') { $flags += @('-O0', '-g', '-D_GLIBCXX_ASSERTIONS') }
else { $flags += @('-O2', '-DNDEBUG') }
$objectPath = Join-Path $BuildDirectory "reference_$Configuration.o"
Invoke-Checked $Compiler ($flags + @('-I', (Join-Path $projectRoot 'include'), '-c', (Join-Path $projectRoot 'src/reference.cpp'), '-o', $objectPath))
foreach ($target in @('baseline_examples', 'reference_tests', 'reference_fixture_driver')) {
    $binaryPath = Join-Path $BuildDirectory "${target}_$Configuration.exe"
    $arguments = $flags + @('-I', (Join-Path $projectRoot 'include'), (Join-Path $projectRoot "tests/$target.cpp"))
    if ($target -ne 'baseline_examples') { $arguments += $objectPath }
    Invoke-Checked $Compiler ($arguments + @('-o', $binaryPath))
    if ($target -ne 'reference_fixture_driver') { Invoke-Checked $binaryPath @() }
}
Invoke-Checked 'python' @((Join-Path $projectRoot 'tools/verify_baseline.py'), '--historical-root', $HistoricalRoot)
Invoke-Checked 'python' @((Join-Path $projectRoot 'tools/verify_reference.py'), '--driver', (Join-Path $BuildDirectory "reference_fixture_driver_$Configuration.exe"))
Write-Output "PASS: complete available $Configuration verification suite"
