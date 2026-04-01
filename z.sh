#!/usr/bin/env bash
# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.

# Thin wrapper that delegates to the nanvix-zutil CLI.
# Self-bootstraps nanvix-zutil into .nanvix/venv/ if it is not already installed.

set -euo pipefail

ZUTIL_VERSION="${NANVIX_ZUTIL_VERSION:-0.5.1}"
REPO_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd -P)"
VENV="$REPO_ROOT/.nanvix/venv"
VENV_BIN="$VENV/bin/nanvix-zutil"
ZUTIL_GLOBAL_VERSION="$(nanvix-zutil --version 2>/dev/null || true )"

function bootstrap() {
	# Pin nanvix-zutil version for reproducible bootstrapping.
	# Override with NANVIX_ZUTIL_VERSION env var if needed.
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
}

# Prefer the venv copy if it exists; otherwise use the global install.
BIN=""
if [ ! -d "$VENV" ] && [ -z "$ZUTIL_GLOBAL_VERSION" ]; then
    bootstrap
    BIN="$VENV_BIN"
elif [ -x "$VENV_BIN" ]; then
    BIN="$VENV_BIN"
elif [ -d "$VENV" ] && ! command -v nanvix-zutil &>/dev/null; then
    echo "Warning: incomplete venv detected (binary missing). Re-running bootstrap..." >&2
    bootstrap
    BIN="$VENV_BIN"
else
    BIN="nanvix-zutil"
    if [ "$ZUTIL_GLOBAL_VERSION" != "nanvix-zutil ${ZUTIL_VERSION}" ]; then
        echo "Warning: nanvix-zutil global install does not match expected version. Expected ${ZUTIL_VERSION}, found ${ZUTIL_GLOBAL_VERSION}." >&2
    fi
fi

exec "$BIN" "$@"
