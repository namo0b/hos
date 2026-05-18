$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$MainSource = Join-Path $ProjectRoot "test.c"
$ModuleSources = Get-ChildItem -Path (Join-Path $ProjectRoot "src") -Filter "*.c" | Sort-Object Name
$BinDir = Join-Path $ProjectRoot "bin"
$OutputFile = Join-Path $BinDir "test.exe"
$GccTempDir = Join-Path $env:PUBLIC "gcc-temp"

New-Item -ItemType Directory -Path $BinDir -Force | Out-Null
New-Item -ItemType Directory -Path $GccTempDir -Force | Out-Null

$env:TMP = $GccTempDir
$env:TEMP = $GccTempDir
$env:TMPDIR = $GccTempDir

gcc -fdiagnostics-color=always -g $MainSource $ModuleSources.FullName -o $OutputFile

Write-Host "Build complete: $OutputFile"
