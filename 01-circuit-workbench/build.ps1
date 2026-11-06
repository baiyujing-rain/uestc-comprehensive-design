param([switch]$Test)
$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsRoot = & $vswhere -latest -products '*' -property installationPath
if (-not $vsRoot) { throw '未找到 Visual Studio C++ 工具链。' }
$msbuild = Join-Path $vsRoot 'MSBuild\Current\Bin\MSBuild.exe'
& $msbuild (Join-Path $projectRoot 't1.vcxproj') /p:Configuration=Debug /p:Platform=x64 /p:PreferredToolArchitecture=x64 /p:LinkIncremental=false /m /v:minimal /nologo
if ($LASTEXITCODE -ne 0) { throw '编译失败，请检查上方输出。' }
$exePath = Join-Path $projectRoot 'x64\Debug\t1.exe'
$binPath = Join-Path $projectRoot 'bin'
New-Item -ItemType Directory -Path $binPath -Force | Out-Null
Copy-Item -LiteralPath $exePath -Destination (Join-Path $binPath 'CircuitWorkbench.exe') -Force
if ($Test) {
    $report = Join-Path $binPath 'regression.txt'
    $testProc = Start-Process -FilePath $exePath -ArgumentList '--self-test',('"' + $report + '"') -WindowStyle Hidden -PassThru
    if (-not $testProc.WaitForExit(55000)) { throw '自检超时。' }
    Get-Content -LiteralPath $report
    if ($testProc.ExitCode -ne 0) { throw '自检失败。' }
}
Write-Host "构建完成：$exePath"
