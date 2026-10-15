# Riddick VR local 24/7 worker (Windows). Runs every 15 min via Task Scheduler.
# Install: schtasks /create /tn "RiddickVRWorker" /tr "powershell -ExecutionPolicy Bypass -File C:\Users\GigelCastel\riddick-vr\tools\vr-worker.ps1" /sc minute /mo 15 /ru "%USERNAME%"
# What it does: pull latest, triage issues, build, semi-play (with display-down backoff), log.
[CmdletBinding()] param([int]$Frames = 120, [int]$TimeoutSec = 90)
$ErrorActionPreference = "Continue"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $Root
$stamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
"=== VR worker tick $stamp ===" | Tee-Object -FilePath "$Root\tools\worker.log" -Append
git pull --ff-only 2>&1 | Select-Object -First 5 | Tee-Object -FilePath "$Root\tools\worker.log" -Append
gh issue list --state open --limit 5 --json number,title 2>&1 | Tee-Object -FilePath "$Root\tools\worker.log" -Append
powershell -ExecutionPolicy Bypass -File "$Root\tools\build-all.ps1" -Config Release 2>&1 | Select-Object -Last 8 | Tee-Object -FilePath "$Root\tools\worker.log" -Append
# Display-down backoff (GH #41): if the game never reaches GL init 3 ticks in
# a row, skip launching it (build still runs) until a launch succeeds again.
$stateFile = "$Root\tools\worker-state.json"
$state = @{ stuck = 0 }
if (Test-Path $stateFile) {
    try { $state = Get-Content $stateFile -Raw | ConvertFrom-Json } catch { $state = @{ stuck = 0 } }
}
if ($state.stuck -ge 3) {
    "SKIP game launch: $($state.stuck) consecutive pre-GL stalls (display down?) - build only this tick" | Tee-Object -FilePath "$Root\tools\worker.log" -Append
    # Probe cheaply: is the display back? A 20s mini-run decides without thrash.
    python3 "$Root\tools\semi_play.py" --frames 15 --interval 15 --timeout 20 2>&1 | Select-Object -Last 6 | Tee-Object -FilePath "$Root\tools\worker.log" -Append
    if ($LASTEXITCODE -eq 0) { $state.stuck = 0 } else { $state.stuck = $state.stuck + 1 }
} else {
    python3 "$Root\tools\semi_play.py" --frames $Frames --interval 15 --timeout $TimeoutSec 2>&1 | Select-Object -Last 25 | Tee-Object -FilePath "$Root\tools\worker.log" -Append
    if ($LASTEXITCODE -eq 0) { $state.stuck = 0 } else { $state.stuck = $state.stuck + 1 }
}
$state | ConvertTo-Json | Set-Content $stateFile
"=== tick end (stuck=$($state.stuck)) ===" | Tee-Object -FilePath "$Root\tools\worker.log" -Append
