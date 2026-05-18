$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$MainSource = Join-Path $ProjectRoot "web_main.c"
$ModuleSources = Get-ChildItem -Path (Join-Path $ProjectRoot "src") -Filter "*.c" | Sort-Object Name
$WebSources = Get-ChildItem -Path (Join-Path $ProjectRoot "web") -Filter "*.c" | Sort-Object Name
$BinDir = Join-Path $ProjectRoot "bin"
$OutputFile = Join-Path $BinDir "hospital_web.exe"
$GccTempDir = Join-Path $env:PUBLIC "gcc-temp"

New-Item -ItemType Directory -Path $BinDir -Force | Out-Null
New-Item -ItemType Directory -Path $GccTempDir -Force | Out-Null

$env:TMP = $GccTempDir
$env:TEMP = $GccTempDir
$env:TMPDIR = $GccTempDir

gcc -fdiagnostics-color=always -g $MainSource $ModuleSources.FullName $WebSources.FullName -lws2_32 -o $OutputFile

Write-Host "Web build complete: $OutputFile"
