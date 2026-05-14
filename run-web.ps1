$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$OutputFile = Join-Path $ProjectRoot "bin\hospital_web.exe"

& (Join-Path $ProjectRoot "build-web.ps1")

Push-Location $ProjectRoot
try {
    & $OutputFile
}
finally {
    Pop-Location
}
