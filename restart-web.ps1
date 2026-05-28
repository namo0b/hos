$ErrorActionPreference = "Stop"

# 기존 웹 서버를 종료하고 다시 빌드한 뒤 8080번 포트로 실행하는 재시작 스크립트입니다.
# Visual Studio 터미널에서 서버를 완전히 다시 켤 때 사용합니다.

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildScript = Join-Path $ProjectRoot "build-web.ps1"
$OutputFile = Join-Path $ProjectRoot "bin\hospital_web.exe"

$ExistingServers = Get-Process -Name "hospital_web" -ErrorAction SilentlyContinue
if ($ExistingServers) {
    # 이미 실행 중인 서버가 있으면 포트 충돌을 막기 위해 먼저 종료합니다.
    $ExistingServers | Stop-Process -Force
    Start-Sleep -Milliseconds 500
}

# 최신 코드가 반영되도록 웹 서버를 다시 빌드합니다.
& $BuildScript

Push-Location $ProjectRoot
try {
    Write-Host ""
    Write-Host "Server starting at http://localhost:8080"
    Write-Host "Press Ctrl+C in this terminal to stop."
    Write-Host ""
    # 이 명령이 실행되는 동안 터미널은 서버 프로세스를 계속 유지합니다.
    & $OutputFile
}
finally {
    Pop-Location
}
