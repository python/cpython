#!/usr/bin/env bash
# compare-conf.sh — run ./configure and ./configure-old, diff their outputs.
#
# Usage: ./compare-conf.sh [configure args...]
# Extra args are passed to both configure scripts.

set -eu

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

NEW_DIR=$(mktemp -d /tmp/conf-new.XXXXXX)
OLD_DIR=$(mktemp -d /tmp/conf-old.XXXXXX)

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

if diff -wbB /dev/null /dev/null 2>/dev/null; then
    DIFF_OPTS='-uwbB'
else
    # not supported on OpenBSD
    DIFF_OPTS='-uwb'
fi

echo "=== Running ./configure $* ==="
rm -f config.cache
PYTHON_FOR_CONFIGURE=python3 ./configure "$@" > "$NEW_DIR/stdout" 2> "$NEW_DIR/stderr" || true
for f in $FILES; do
    mkdir -p "$NEW_DIR/$(dirname "$f")"
    [ -f "$f" ] && cp "$f" "$NEW_DIR/$f" || touch "$NEW_DIR/$f"
done

echo "=== Running ./configure-old $* ==="
rm -f config.cache
./configure-old "$@" > "$OLD_DIR/stdout" 2> "$OLD_DIR/stderr" || true
for f in $FILES; do
    mkdir -p "$OLD_DIR/$(dirname "$f")"
    [ -f "$f" ] && cp "$f" "$OLD_DIR/$f" || touch "$OLD_DIR/$f"
done

echo ""
DIFFS=0
for f in $FILES; do
    echo "=== diff $f ==="
    if diff $DIFF_OPTS "$OLD_DIR/$f" "$NEW_DIR/$f"; then
        echo "(no differences)"
    else
        DIFFS=$((DIFFS + 1))
    fi
    echo ""
done

if [ "$DIFFS" -eq 0 ]; then
    echo "All outputs match."
else
    echo "$DIFFS file(s) differ."
    exit 1
fi
