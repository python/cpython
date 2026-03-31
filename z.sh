#!/usr/bin/env bash
# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.

# Thin wrapper that delegates to the nanvix-zutil CLI.
# Self-bootstraps nanvix-zutil into .nanvix/venv/ if it is not already installed.

set -euo pipefail

# If nanvix-zutil is already on PATH (e.g. CI), use it directly.
if command -v nanvix-zutil &>/dev/null; then
exec nanvix-zutil "$@"
fi

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd -P)"
# z.sh lives at the repository root, so use its directory directly
# instead of relying on git to discover the top-level checkout directory.
REPO_ROOT="$SCRIPT_DIR"

VENV="$REPO_ROOT/.nanvix/venv"

if ! "$VENV/bin/nanvix-zutil" --version &>/dev/null; then
# Pin nanvix-zutil version for reproducible bootstrapping.
# Override with NANVIX_ZUTIL_VERSION env var if needed.
ZUTIL_VERSION="${NANVIX_ZUTIL_VERSION:-0.4.2}"
echo "nanvix-zutil not found — bootstrapping nanvix-zutil==${ZUTIL_VERSION}..." >&2

if ! command -v python3 &>/dev/null; then
echo "Error: python3 not found. Install Python 3 and ensure python3 is on PATH." >&2
exit 1
fi

WHEEL_URL="https://github.com/nanvix/zutils/releases/download/v${ZUTIL_VERSION}/nanvix_zutil-${ZUTIL_VERSION}-py3-none-any.whl"
if [ -d "$VENV" ]; then
python3 -m venv --clear "$VENV"
else
python3 -m venv "$VENV"
fi
"$VENV/bin/pip" install --quiet "$WHEEL_URL"
fi

# Prefer the venv copy if it exists; otherwise use the global install.
if [ -x "$VENV/bin/nanvix-zutil" ]; then
exec "$VENV/bin/nanvix-zutil" "$@"
elif command -v nanvix-zutil &>/dev/null; then
exec nanvix-zutil "$@"
else
echo "nanvix-zutil not found in venv ($VENV) or on PATH." >&2
exit 1
fi
