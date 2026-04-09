#!/bin/sh
# sync-sources.sh - Content-aware source sync for incremental Docker builds.
#
# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.
#
# Extracts host workspace sources into a staging area, normalises CRLF line
# endings on autotools files, then copies only genuinely changed files into
# the persistent build directory.  Unchanged files keep their original
# timestamps so Make's dependency tracking works correctly.
#
# Usage:  sync-sources.sh <ws_mount> <build_dir> [crlf_files...]
#
# Environment:
#   TAR_EXCLUDES  - tar --exclude flags (word-split intentionally)

set -e

WS_MOUNT="$1"; shift
BUILD_DIR="$1"; shift
# Remaining positional args are files requiring CRLF -> LF normalization.

_stg=$(mktemp -d /tmp/sync-staging.XXXXXX)
_sync_list=$(mktemp /tmp/sync-list.XXXXXX)
_build_list=$(mktemp /tmp/build-list.XXXXXX)
# Clean up temp files on exit (normal or early error via set -e).
trap 'rm -rf "$_stg" "$_sync_list" "$_build_list"' EXIT

# Word splitting on TAR_EXCLUDES is intentional (multiple --exclude flags).
# shellcheck disable=SC2086
tar -cf - -C "$WS_MOUNT" ${TAR_EXCLUDES} . | tar -xf - -C "$_stg"

for f in "$@"; do
    [ -f "$_stg/$f" ] && sed -i 's/\r$//' "$_stg/$f"
done

# --- Copy new/changed files into the build directory ---
cd "$_stg"
find . -type f > "$_sync_list"
while IFS= read -r _f; do
    _t="$BUILD_DIR/$_f"
    if [ ! -f "$_t" ] || ! cmp -s "$_f" "$_t"; then
        mkdir -p "$(dirname "$_t")"
        cp -f "$_f" "$_t"
    fi
done < "$_sync_list"

# --- Remove stale files from the build directory ---
# Files that exist in the persistent build tree but no longer appear in the
# source workspace must be pruned so they don't affect subsequent builds.
cd "$BUILD_DIR"
find . -type f > "$_build_list"
while IFS= read -r _f; do
    [ ! -f "$_stg/$_f" ] && rm -f "$BUILD_DIR/$_f"
done < "$_build_list"
