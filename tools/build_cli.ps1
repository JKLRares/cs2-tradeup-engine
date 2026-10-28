param(
    [string]$Compiler = 'g++',
    [string]$BuildDirectory = '.dist/milestone4',
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [switch]$ApplicationTests
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$projectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if (-not [System.IO.Path]::IsPathRooted($BuildDirectory)) { $BuildDirectory = Join-Path $projectRoot $BuildDirectory }
$BuildDirectory = [System.IO.Path]::GetFullPath($BuildDirectory)
foreach ($component in $BuildDirectory.Split([char[]]'\/')) {
    if ($component -ieq 'historical' -or $component -ieq '.git') { throw 'Build output must be outside historical materials and .git.' }
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
$includes = @('-I', (Join-Path $projectRoot 'include'))
$objects = @()
foreach ($source in @('reference', 'optimized', 'sha256', 'catalog_io', 'historical', 'application', 'serialization')) {
    $objectPath = Join-Path $BuildDirectory "${source}_$Configuration.o"
    Invoke-Checked $Compiler ($flags + $includes + @('-c', (Join-Path $projectRoot "src/$source.cpp"), '-o', $objectPath))
    $objects += $objectPath
}
$binaryPath = Join-Path $BuildDirectory "tradeup_$Configuration.exe"
Invoke-Checked $Compiler ($flags + $includes + @((Join-Path $projectRoot 'src/cli.cpp')) + $objects + @('-municode', '-o', $binaryPath))
if ($ApplicationTests) {
    $testBinary = Join-Path $BuildDirectory "application_tests_$Configuration.exe"
    Invoke-Checked $Compiler ($flags + $includes + @((Join-Path $projectRoot 'tests/application_tests.cpp')) + $objects + @('-o', $testBinary))
    Invoke-Checked $testBinary @()
}
Write-Output "Built $binaryPath"
