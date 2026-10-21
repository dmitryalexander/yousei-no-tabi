param([string]$BuildDir = (Join-Path $PSScriptRoot '../build/lighting-tests'))
$ErrorActionPreference = 'Stop'
$releaseSourceRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$releaseTestOutput = [System.IO.Path]::GetFullPath($BuildDir)
if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    throw 'Run this script from a Visual Studio x64 Native Tools developer shell.'
}
$releaseTestResults = @()
foreach ($releaseTestName in @('presentation_rate','shader_cache','relief_cache','prepared_source','prewarm_cache_concurrency')) {
    $releaseCaseDir = Join-Path $releaseTestOutput $releaseTestName
    New-Item -ItemType Directory -Path $releaseCaseDir -Force | Out-Null
    Push-Location $releaseCaseDir
    try {
        & cl.exe /nologo /std:c++20 /EHsc /O2 /UNDEBUG "/I$releaseSourceRoot/libultraship/include" "/Fe:$releaseTestName.exe" "/Fo:$releaseTestName.obj" (Join-Path $PSScriptRoot "test_$releaseTestName.cpp")
        if ($LASTEXITCODE -ne 0) { throw "Compilation failed: $releaseTestName" }
        & (Join-Path $releaseCaseDir "$releaseTestName.exe")
        if ($LASTEXITCODE -ne 0) { throw "Test failed: $releaseTestName" }
        $releaseTestResults += @{ test = $releaseTestName; passed = $true }
    } finally { Pop-Location }
}
$releaseTestResults | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $releaseTestOutput 'results.json')
