# Phase 1 test runner for PowerShell users.
#
# Wraps tests/run_tests.sh so you can just do:
#   powershell -File tests\run_tests.ps1
#   .\tests\run_tests.ps1        (if execution policy allows)
#
# It does NOT need WSL — it uses the MSYS2 bash that's already installed
# (same one the build uses). The .sh script handles adding ucrt64's runtime
# DLLs to PATH itself.

$ErrorActionPreference = 'Stop'

# Repo root = parent of this script's folder
$repoRoot = Split-Path -Parent $PSScriptRoot
$script   = Join-Path $repoRoot 'tests\run_tests.sh'

if (-not (Test-Path $script)) {
    Write-Error "test script not found: $script"
    exit 1
}

# Find an MSYS2-style bash (not WSL's System32 bash, which would need WSL set up).
$candidates = @(
    'C:\msys64\usr\bin\bash.exe',
    'C:\Program Files\Git\bin\bash.exe',
    'C:\Program Files\Git\usr\bin\bash.exe'
)
$bash = $null
foreach ($c in $candidates) {
    if (Test-Path $c) { $bash = $c; break }
}
if (-not $bash) {
    # last resort: whatever 'bash' resolves to, unless it's the WSL stub
    $cmd = Get-Command bash.exe -ErrorAction SilentlyContinue
    if ($cmd -and $cmd.Source -notlike '*System32*') { $bash = $cmd.Source }
}
if (-not $bash) {
    Write-Error "no MSYS2/Git bash found. Install MSYS2 or Git for Windows."
    exit 1
}

Write-Host "using bash: $bash"

# Convert D:\path\to\repo -> /d/path/to/repo for bash
$drive = $repoRoot.Substring(0,1).ToLower()
$rest  = $repoRoot.Substring(2) -replace '\\', '/'
$posixRoot = "/$drive$rest"

& $bash -lc "cd '$posixRoot' && bash tests/run_tests.sh"
exit $LASTEXITCODE
