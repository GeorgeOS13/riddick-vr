#requires -Version 5.1
<#
  RiddickVR Autoloop runner.
  Launches one unattended OpenCode run with the standing prompt.
  Single-instance via a named mutex; logs each run to logs/.
  Registered as scheduled task "RiddickVR-Autoloop" (every 10 min, 24h).
  Disable:  schtasks /Delete /TN "RiddickVR-Autoloop" /F
#>

# Model used by the unattended loop. Change here if a different model is wanted.
$model = 'opencode-go/deepseek-v4.1-flash'

$repo   = 'C:\Users\GigelCastel\riddick-vr'
$dir    = Join-Path $repo 'tools\autoloop'
$logDir = Join-Path $dir 'logs'
$stateLog = Join-Path $logDir 'STATE.log'
$opencode = 'C:\Program Files\nodejs\opencode.cmd'

# Deep RE: make the extracted Ghidra visible to `rea` inside the loop. (setx is
# user-level; a task launched by the scheduler may not inherit the edited env.)
$env:GHIDRA_INSTALL_DIR = 'C:\Users\GigelCastel\Tools\ghidra_12.1.4_PUBLIC'

New-Item -ItemType Directory -Force -Path $logDir | Out-Null

# Single instance: skip if a run is already in progress.
$mtx = New-Object System.Threading.Mutex($false, 'Local\RiddickVRAutoloop')
if (-not $mtx.WaitOne(0)) {
    "[{0}] skip: previous run still active" -f (Get-Date -Format o) |
        Add-Content -Path $stateLog
    exit 0
}

try {
    $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
    $outFile = Join-Path $logDir ("run-{0}.log" -f $stamp)
    $errFile = Join-Path $logDir ("run-{0}.err.log" -f $stamp)
    $prompt = Get-Content (Join-Path $dir 'prompt.md') -Raw

    Set-Location $repo
    "[{0}] start run" -f (Get-Date -Format o) | Add-Content -Path $stateLog

    # Native command stderr must not abort the script under Stop semantics.
    $prevEap = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $ocArgs = @(
            'run', '--auto', '--standalone', '--model', $model,
            '--title', ("RiddickVR autoloop {0}" -f $stamp),
            $prompt
        )
        & $opencode @ocArgs 1> $outFile 2> $errFile
        $code = $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $prevEap
    }

    "[{0}] end run exit={1} log={2}" -f (Get-Date -Format o), $code, (Split-Path $outFile -Leaf) |
        Add-Content -Path $stateLog
}
finally {
    $mtx.ReleaseMutex()
    $mtx.Dispose()
}
