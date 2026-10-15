# Push-only safety net: keeps origin/main current even if the autoloop stalls.
# Read-only w.r.t. the working tree; normal (never forced) pushes only.
$repo = 'C:\Users\GigelCastel\riddick-vr'
Set-Location $repo
$log = Join-Path $repo 'tools\autoloop\logs\push.log'
New-Item -ItemType Directory -Force -Path (Split-Path $log) | Out-Null
try {
    git fetch origin --quiet 2>$null
    $ahead = [int](git rev-list --count 'origin/main..main' 2>$null)
    if ($ahead -gt 0) {
        $out = git push origin main 2>&1
        "[{0}] pushed {1} commit(s): {2}" -f (Get-Date -Format o), $ahead, ($out -join ' ') |
            Add-Content -Path $log
    }
} catch {
    "[{0}] push-main error: {1}" -f (Get-Date -Format o), $_ | Add-Content -Path $log
}
