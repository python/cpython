#!/bin/sh
# compare-transpile.sh — transpile configure.py to shell, run it, run
# configure-old, and diff their outputs.
#
# Usage: ./compare-transpile.sh [configure args...]
# Extra args are passed to both configure scripts.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

NEW_DIR=$(mktemp -d /tmp/conf-transpile.XXXXXX)
OLD_DIR=$(mktemp -d /tmp/conf-old.XXXXXX)
TRANSPILED="$NEW_DIR/configure"

cleanup() { rm -rf "$NEW_DIR" "$OLD_DIR"; }
trap cleanup EXIT

FILES="
    Misc/python.pc
    Misc/python-embed.pc
    Misc/python-config.sh
    Modules/config.c
    Modules/Setup.bootstrap
    Modules/Setup.stdlib
    Modules/ld_so_aix
    Makefile.pre
    pyconfig.h
"

# --- Step 1: Transpile ---
echo "=== Transpiling configure.py -> shell ==="
uv run Tools/configure/transpile.py -o "$TRANSPILED"
echo "  wrote $TRANSPILED ($(wc -l < "$TRANSPILED") lines)"

# --- Step 2: Run transpiled configure ---
#TEST_SHELL=$(command -v dash 2>/dev/null || true)
#if [ -z "$TEST_SHELL" ]; then
#    echo "warning: dash not found, using /bin/sh" >&2
#    TEST_SHELL=/bin/sh
#fi
if test -e /bin/bash; then
    TEST_SHELL=/bin/bash
else
    TEST_SHELL=/bin/sh
fi

echo "=== Running transpiled configure ($TEST_SHELL) $* ==="
rm -f config.cache
if ! "$TEST_SHELL" "$TRANSPILED" "$@" > "$NEW_DIR/stdout" 2> "$NEW_DIR/stderr"; then
    echo "New configure script crashed, aborting compare."
    cat < "$NEW_DIR/stderr"
    exit 1
fi
for f in $FILES; do
    mkdir -p "$NEW_DIR/$(dirname "$f")"
    if [ -f "$f" ]; then cp "$f" "$NEW_DIR/$f"; else touch "$NEW_DIR/$f"; fi
done

# --- Step 3: Run configure-old ---
echo "=== Running ./configure-old $* ==="
rm -f config.cache
"$TEST_SHELL" ./configure-old "$@" > "$OLD_DIR/stdout" 2> "$OLD_DIR/stderr"
for f in $FILES; do
    mkdir -p "$OLD_DIR/$(dirname "$f")"
    if [ -f "$f" ]; then cp "$f" "$OLD_DIR/$f"; else touch "$OLD_DIR/$f"; fi
done

# --- Step 4: Compare ---
echo ""
DIFFS=0
for f in $FILES; do
    echo "=== diff $f ==="
    if diff -u -wbB "$OLD_DIR/$f" "$NEW_DIR/$f"; then
        echo "(no differences)"
    else
        DIFFS=$((DIFFS + 1))
    fi
    echo ""
done

# Show stderr from the transpiled run (warnings/errors are useful for debugging)
if [ -s "$NEW_DIR/stderr" ]; then
    echo "=== transpiled configure stderr ==="
    cat "$NEW_DIR/stderr"
    echo ""
fi

if [ "$DIFFS" -eq 0 ]; then
    echo "All outputs match."
else
    echo "$DIFFS file(s) differ."
    exit 1
fi
