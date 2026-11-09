$ErrorActionPreference = 'Stop'
$cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
$cmakePath = if ($cmakeCommand) { $cmakeCommand.Source } else {
    $locator = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (!(Test-Path $locator)) { throw 'Installer CMake et Visual Studio Build Tools C++.' }
    & $locator -latest -products '*' -find '**\cmake.exe' | Select-Object -First 1
}
if (!$cmakePath) { throw 'CMake introuvable.' }
# Reuse a JUCE copy already on this PC when there is one (avoids downloading it again).
$localJuce = Join-Path $PSScriptRoot '..\swell-knock\build\_deps\juce-src'
$juceArg = if (Test-Path $localJuce) { "-DFETCHCONTENT_SOURCE_DIR_JUCE=$((Resolve-Path $localJuce).Path.Replace('\','/'))" } else { '-DUNUSED_JUCE_HINT=1' }
& $cmakePath -S $PSScriptRoot -B (Join-Path $PSScriptRoot 'build') -G 'Visual Studio 17 2022' -A x64 $juceArg
if ($LASTEXITCODE) { throw 'Configuration échouée.' }
& $cmakePath --build (Join-Path $PSScriptRoot 'build') --config Release --parallel 3
if ($LASTEXITCODE) { throw 'Compilation échouée.' }
& (Join-Path (Split-Path $cmakePath) 'ctest.exe') --test-dir (Join-Path $PSScriptRoot 'build') -C Release --output-on-failure
if ($LASTEXITCODE) { throw 'Tests échoués.' }
