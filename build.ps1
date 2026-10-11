# Root build launcher forwarding to tools/build-all.ps1
param(
    [string]$Config = "Release",
    [switch]$Clean
)

$toolsScript = Join-Path $PSScriptRoot "tools\build-all.ps1"
& $toolsScript -Config $Config -Clean:$Clean
