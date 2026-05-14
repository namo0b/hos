$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$OutputFile = Join-Path $ProjectRoot "bin\test.exe"

& (Join-Path $ProjectRoot "build.ps1")

Push-Location $ProjectRoot
try {
    & $OutputFile
}
finally {
    Pop-Location
}
