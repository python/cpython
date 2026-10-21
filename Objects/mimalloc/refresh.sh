#!/usr/bin/env bash
#
# Use this script to update the vendored mimalloc sources from a local
# checkout of the upstream mimalloc repository.
#
# The sources in src/ are copied to Objects/mimalloc/ and the headers in
# include/ are copied to Include/internal/mimalloc/. Files that no longer exist
# upstream are removed. Only files tracked by git in the mimalloc checkout are
# copied, so build output and other local files are ignored.
#
# CPython-specific configuration lives in Include/internal/pycore_mimalloc.h,
# which this script does not touch.

set -e
set -o pipefail

if [[ "${BASH_VERSINFO[0]}" -lt 4 ]]; then
  echo "A bash version >= 4 required. Got: $BASH_VERSION" >&2
  exit 1
fi

if [[ $1 == "" ]]; then
  echo "Usage: $0 path-to-mimalloc-directory"
  echo ""
  echo "  path-to-mimalloc-directory should be a local git checkout of a"
  echo "  https://github.com/microsoft/mimalloc/ repo (dev3 branch), with the"
  echo "  revision to vendor checked out."
  exit 1
fi

mimalloc_dir="$(realpath "$1")"
objects_dir="$(realpath "$(dirname -- "${BASH_SOURCE[0]}")")"
cpython_dir="$(realpath "$objects_dir/../..")"
include_dir="$cpython_dir/Include/internal/mimalloc"
script_name="$(basename -- "${BASH_SOURCE[0]}")"

if [[ ! -f "$mimalloc_dir/src/static.c" || ! -f "$mimalloc_dir/include/mimalloc.h" ]]; then
  echo "error: '$mimalloc_dir' does not look like a mimalloc checkout" >&2
  echo "(expected src/static.c and include/mimalloc.h)" >&2
  exit 1
fi
if [[ ! -f "$cpython_dir/Include/internal/pycore_mimalloc.h" ]]; then
  echo "error: '$cpython_dir' does not look like a CPython checkout" >&2
  exit 1
fi

rev=$(git -C "$mimalloc_dir" rev-parse HEAD)
describe=$(git -C "$mimalloc_dir" describe --tags --always 2>/dev/null || echo "$rev")
if [[ -n "$(git -C "$mimalloc_dir" status --porcelain -- src include LICENSE)" ]]; then
  echo "WARNING: '$mimalloc_dir' has uncommitted changes in src/, include/" >&2
  echo "or LICENSE; the working tree contents will be vendored." >&2
fi

# Step 1: remove the currently vendored files (keeping this script)

find "$objects_dir" -type f ! -name "$script_name" -delete
find "$objects_dir" -mindepth 1 -type d -empty -delete
rm -rf "$include_dir"
mkdir -p "$include_dir"

# Step 2: copy the tracked upstream files

copy_tree() {
  local src_subdir="$1" dest_dir="$2" f rel
  while IFS= read -r -d '' f; do
    rel="${f#"$src_subdir"/}"
    mkdir -p "$dest_dir/$(dirname -- "$rel")"
    cp "$mimalloc_dir/$f" "$dest_dir/$rel"
  done < <(git -C "$mimalloc_dir" ls-files -z -- "$src_subdir")
}

copy_tree src "$objects_dir"
copy_tree include "$include_dir"
cp "$mimalloc_dir/LICENSE" "$objects_dir/LICENSE"

# Step 3: sanity checks

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

echo "Updated mimalloc to $describe ($rev)."
echo
git status --short -- Objects/mimalloc Include/internal/mimalloc
echo
echo "Review the changes, rebuild, and commit with a message such as:"
echo "  Update vendored mimalloc to $describe"
