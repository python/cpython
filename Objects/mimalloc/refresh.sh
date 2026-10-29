#!/usr/bin/env bash
#
# Use this script to update the vendored mimalloc sources.
#
# The sources in src/ are copied to Objects/mimalloc/ and the headers in
# include/ are copied to Include/internal/mimalloc/. Files that no longer exist
# upstream are removed.
#
# CPython-specific configuration lives in Include/internal/pycore_mimalloc.h,
# which this script does not touch. Changes carried on top of the upstream
# sources must be re-applied after refreshing.

set -e
set -o pipefail

if [[ "${BASH_VERSINFO[0]}" -lt 4 ]]; then
  echo "A bash version >= 4 required. Got: $BASH_VERSION" >&2
  exit 1
fi

# Update this when updating to a new version after verifying that the changes
# the update brings in are good. The ref can be a commit hash or a release tag
# (e.g. "v3.6.0"); mimalloc v3.6 has not been released yet, so this is a commit
# on the dev3 branch.
expected_mimalloc_ref="caaf3bc41494e6b58ab32ed9c851b3c8b61a8f82"
expected_mimalloc_sha256="d6bed4bad2fe8aea63f4ab25cabce5275aef6f2f9386266ff686ceb1eabe15bb"

objects_dir="$(realpath "$(dirname -- "${BASH_SOURCE[0]}")")"
cpython_dir="$(realpath "$objects_dir/../..")"
include_dir="$cpython_dir/Include/internal/mimalloc"
script_name="$(basename -- "${BASH_SOURCE[0]}")"

tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

# Step 1: download and verify the mimalloc sources

url="https://github.com/microsoft/mimalloc/archive/${expected_mimalloc_ref}.tar.gz"
curl --fail --location "$url" > "$tmp_dir/mimalloc.tar.gz"
actual_sha256=$(sha256sum "$tmp_dir/mimalloc.tar.gz" | cut -d' ' -f1)
if [[ "$actual_sha256" != "$expected_mimalloc_sha256" ]]; then
  echo "error: SHA-256 mismatch for $url" >&2
  echo "  expected: $expected_mimalloc_sha256" >&2
  echo "  actual:   $actual_sha256" >&2
  echo "Update expected_mimalloc_sha256 after verifying the download." >&2
  exit 1
fi

# The top-level directory in the archive depends on the ref (mimalloc-<hash>
# for a commit, mimalloc-<version> for a tag), so strip it.
mimalloc_dir="$tmp_dir/mimalloc"
mkdir "$mimalloc_dir"
tar xzf "$tmp_dir/mimalloc.tar.gz" -C "$mimalloc_dir" --strip-components 1
if [[ ! -f "$mimalloc_dir/src/static.c" || ! -f "$mimalloc_dir/include/mimalloc.h" ]]; then
  echo "error: $url does not look like a mimalloc source archive" >&2
  exit 1
fi

# Step 2: remove the currently vendored files (keeping this script)

find "$objects_dir" -type f ! -name "$script_name" -delete
find "$objects_dir" -mindepth 1 -type d -empty -delete
rm -rf "$include_dir"
mkdir -p "$include_dir"

# Step 3: copy the upstream files

cp -R "$mimalloc_dir/src/." "$objects_dir/"
cp -R "$mimalloc_dir/include/." "$include_dir/"
cp "$mimalloc_dir/LICENSE" "$objects_dir/LICENSE"

# Step 4: sanity checks

cd "$cpython_dir"

# pre-commit rejects trailing whitespace in C files.
ws_files=$(grep -rlIE '[[:blank:]]+$' Objects/mimalloc Include/internal/mimalloc \
             --include='*.c' --include='*.h' || true)
if [[ -n "$ws_files" ]]; then
  echo "WARNING: these files have trailing whitespace:" >&2
  echo "$ws_files" | sed 's/^/  /' >&2
fi

# Makefile.pre.in lists the mimalloc files with wildcards of a fixed depth;
# files nested deeper are not tracked as build dependencies.
deep_files=$( (find Objects/mimalloc -mindepth 4 -name '*.[ch]';
               find Include/internal/mimalloc -mindepth 4 -name '*.h') || true)
if [[ -n "$deep_files" ]]; then
  echo "WARNING: these files are nested deeper than the wildcards in" >&2
  echo "Makefile.pre.in cover; update MIMALLOC_HEADERS and the" >&2
  echo "Objects/obmalloc.o dependencies:" >&2
  echo "$deep_files" | sed 's/^/  /' >&2
fi

echo "Updated mimalloc to ${expected_mimalloc_ref}."
echo
git status --short -- Objects/mimalloc Include/internal/mimalloc
echo
echo "Re-apply any changes carried on top of the upstream sources, review the"
echo "changes, rebuild, and commit with a message such as:"
echo "  Update vendored mimalloc to ${expected_mimalloc_ref}"
