# Riddick VR local 24/7 worker (Windows). Runs every 15 min via Task Scheduler.
# Install: schtasks /create /tn "RiddickVRWorker" /tr "powershell -ExecutionPolicy Bypass -File C:\Users\GigelCastel\riddick-vr\tools\vr-worker.ps1" /sc minute /mo 15 /ru "%USERNAME%"
# What it does: pull latest, pick oldest open issue, build, semi-play, log. If no open issues, print next REA gap suggestion.
[CmdletBinding()] param([int]$Frames = 120, [int]$TimeoutSec = 90)
$ErrorActionPreference = "Continue"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $Root
$stamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
"=== VR worker tick $stamp ===" | Tee-Object -FilePath "$Root\tools\worker.log" -Append
git pull --ff-only 2>&1 | Select-Object -First 5 | Tee-Object -FilePath "$Root\tools\worker.log" -Append
gh issue list --state open --limit 5 --json number,title 2>&1 | Tee-Object -FilePath "$Root\tools\worker.log" -Append
powershell -ExecutionPolicy Bypass -File "$Root\tools\build-all.ps1" -Config Release 2>&1 | Select-Object -Last 8 | Tee-Object -FilePath "$Root\tools\worker.log" -Append
python3 "$Root\tools\semi_play.py" --frames $Frames --interval 15 --timeout $TimeoutSec 2>&1 | Select-Object -Last 25 | Tee-Object -FilePath "$Root\tools\worker.log" -Append
"=== tick end ===" | Tee-Object -FilePath "$Root\tools\worker.log" -Append
