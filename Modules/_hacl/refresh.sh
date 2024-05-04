#!/usr/bin/env bash
#
# Use this script to update the HACL generated hash algorithm implementation
# code from a local checkout of the upstream hacl-star repository.
#

set -e
set -o pipefail

if [[ "${BASH_VERSINFO[0]}" -lt 4 ]]; then
  echo "A bash version >= 4 required. Got: $BASH_VERSION" >&2
  exit 1
fi

if [[ $1 == "" ]]; then
  echo "Usage: $0 path-to-hacl-directory"
  echo ""
  echo "  path-to-hacl-directory should be a local git checkout of a"
  echo "  https://github.com/hacl-star/hacl-star/ repo."
  exit 1
fi

# Update this when updating to a new version after verifying that the changes
# the update brings in are good.
expected_hacl_star_rev=bb3d0dc8d9d15a5cd51094d5b69e70aa09005ff0

hacl_dir="$(realpath "$1")"
cd "$(dirname "$0")"
actual_rev=$(cd "$hacl_dir" && git rev-parse HEAD)

if [[ "$actual_rev" != "$expected_hacl_star_rev" ]]; then
  echo "WARNING: HACL* in '$hacl_dir' is at revision:" >&2
  echo " $actual_rev" >&2
  echo "but expected revision:" >&2
  echo " $expected_hacl_star_rev" >&2
  echo "Edit the expected rev if the changes pulled in are what you want."
fi

# Step 1: copy files

declare -a dist_files
dist_files=(
  Hacl_Hash_SHA2.h
  Hacl_Streaming_Types.h
  Hacl_Hash_SHA1.h
  internal/Hacl_Hash_SHA1.h
  Hacl_Hash_MD5.h
  Hacl_Hash_SHA3.h
  internal/Hacl_Hash_MD5.h
  internal/Hacl_Hash_SHA3.h
  Hacl_Hash_SHA2.c
  internal/Hacl_Hash_SHA2.h
  Hacl_Hash_SHA1.c
  Hacl_Hash_MD5.c
  Hacl_Hash_SHA3.c
)

declare -a include_files
include_files=(
  include/krml/lowstar_endianness.h
  include/krml/internal/target.h
)

declare -a lib_files
lib_files=(
  krmllib/dist/minimal/FStar_UInt_8_16_32_64.h
  krmllib/dist/minimal/fstar_uint128_struct_endianness.h
  krmllib/dist/minimal/FStar_UInt128_Verified.h
)

# C files for the algorithms themselves: current directory
(cd "$hacl_dir/dist/gcc-compatible" && tar cf - "${dist_files[@]}") | tar xf -

# Support header files (e.g. endianness macros): stays in include/
(cd "$hacl_dir/dist/karamel" && tar cf - "${include_files[@]}") | tar xf -

# Special treatment: we don't bother with an extra directory and move krmllib
# files to the same include directory
for f in "${lib_files[@]}"; do
  cp "$hacl_dir/dist/karamel/$f" include/krml/
done

# Step 2: some in-place modifications to keep things simple and minimal

# This is basic, but refreshes of the vendored HACL code are infrequent, so
# let's not over-engineer this.
if [[ $(uname) == "Darwin" ]]; then
  # You're already running with homebrew or macports to satisfy the
  # bash>=4 requirement, so requiring GNU sed is entirely reasonable.
  sed=gsed
else
  sed=sed
fi

readarray -t all_files < <(find . -name '*.h' -or -name '*.c')

# types.h originally contains a complex series of if-defs and auxiliary type
# definitions; here, we just need a proper uint128 type in scope
# is a simple wrapper that defines the uint128 type
cat > include/krml/types.h <<EOF
#pragma once

#include <inttypes.h>

typedef struct FStar_UInt128_uint128_s {
  uint64_t low;
  uint64_t high;
} FStar_UInt128_uint128, uint128_t;

#define KRML_VERIFIED_UINT128

#include "krml/lowstar_endianness.h"
#include "krml/fstar_uint128_struct_endianness.h"
#include "krml/FStar_UInt128_Verified.h"
EOF
# Adjust the include path to reflect the local directory structure
$sed -i 's!#include.*types.h"!#include "krml/types.h"!g' "${all_files[@]}"
$sed -i 's!#include.*compat.h"!!g' "${all_files[@]}"

# FStar_UInt_8_16_32_64 contains definitions useful in the general case, but not
# for us; trim!
$sed -i -z 's!\(extern\|typedef\)[^;]*;\n\n!!g' include/krml/FStar_UInt_8_16_32_64.h

# This contains static inline prototypes that are defined in
# FStar_UInt_8_16_32_64; they are by default repeated for safety of separate
# compilation, but this is not necessary.
$sed -i 's!#include.*Hacl_Krmllib.h"!!g' "${all_files[@]}"

# Use globally unique names for the Hacl_ C APIs to avoid linkage conflicts.
$sed -i -z 's!#include <string.h>\n!#include <string.h>\n#include "python_hacl_namespaces.h"\n!' Hacl_Hash_*.h

# Finally, we remove a bunch of ifdefs from target.h that are, again, useful in
# the general case, but not exercised by the subset of HACL* that we vendor.
$sed -z -i 's!#ifndef KRML_\(PRE_ALIGN\|POST_ALIGN\|ALIGNED_MALLOC\|ALIGNED_FREE\|HOST_TIME\)\n\(\n\|#  [^\n]*\n\|[^#][^\n]*\n\)*#endif\n\n!!g' include/krml/internal/target.h
$sed -z -i 's!\n\n\([^#][^\n]*\n\)*#define KRML_\(EABORT\|EXIT\)[^\n]*\(\n  [^\n]*\)*!!g' include/krml/internal/target.h
$sed -z -i 's!\n\n\([^#][^\n]*\n\)*#if [^\n]*\n\(  [^\n]*\n\)*#define  KRML_\(EABORT\|EXIT\|CHECK_SIZE\)[^\n]*\(\n  [^\n]*\)*!!g' include/krml/internal/target.h
$sed -z -i 's!\n\n\([^#][^\n]*\n\)*#if [^\n]*\n\(  [^\n]*\n\)*#  define _\?KRML_\(DEPRECATED\|HOST_SNPRINTF\)[^\n]*\n\([^#][^\n]*\n\|#el[^\n]*\n\|#  [^\n]*\n\)*#endif!!g' include/krml/internal/target.h

echo "Updated; verify all is okay using git diff and git status."
