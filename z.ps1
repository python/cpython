# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.

# Thin wrapper that delegates to the nanvix-zutil CLI.
# Self-bootstraps nanvix-zutil into .nanvix\venv\ if it is not already installed.

param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$Args
)

$ErrorActionPreference = 'Stop'

# If nanvix-zutil is already on PATH (e.g. CI), use it directly.
if (Get-Command nanvix-zutil -ErrorAction SilentlyContinue) {
    & nanvix-zutil @Args
    exit $LASTEXITCODE
}

# z.ps1 lives at the repository root, so use its directory directly
# instead of relying on git to discover the top-level checkout directory.
$repoRoot = $PSScriptRoot
$venvDir = Join-Path $repoRoot ".nanvix\venv"
$venvPython = Join-Path $venvDir "Scripts\python.exe"
$venvZutil = Join-Path $venvDir "Scripts\nanvix-zutil.exe"

if (-not (Test-Path $venvZutil)) {
    # Pin nanvix-zutil version for reproducible bootstrapping.
    # Override with NANVIX_ZUTIL_VERSION env var if needed.
    $zutilVersion = if ($env:NANVIX_ZUTIL_VERSION) { $env:NANVIX_ZUTIL_VERSION } else { "0.4.2" }
    Write-Host "nanvix-zutil not found — bootstrapping nanvix-zutil==${zutilVersion}..." -ForegroundColor Yellow

    $wheelUrl = "https://github.com/nanvix/zutils/releases/download/v${zutilVersion}/nanvix_zutil-${zutilVersion}-py3-none-any.whl"

    # Discover a Python 3 interpreter.
    if (Get-Command py -ErrorAction SilentlyContinue) {
        & py -3 -m venv $venvDir
    } elseif (Get-Command python -ErrorAction SilentlyContinue) {
        & python -m venv $venvDir
    } elseif (Get-Command python3 -ErrorAction SilentlyContinue) {
        & python3 -m venv $venvDir
    } else {
        throw "Python 3 not found. Install Python 3 and ensure py, python, or python3 is on PATH."
    }
    & $venvPython -m pip install --quiet $wheelUrl
}

# Prefer the venv copy; fall back to global.
if (Test-Path $venvZutil) {
    & $venvZutil @Args
} elseif (Get-Command nanvix-zutil -ErrorAction SilentlyContinue) {
    & nanvix-zutil @Args
} else {
    throw "nanvix-zutil not found in venv ($venvDir) or on PATH."
}
exit $LASTEXITCODE
