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
expected_hacl_star_rev=4ef25b547b377dcef855db4289c6a00580e7221c

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
  Hacl_Streaming_Types.h
  internal/Hacl_Streaming_Types.h
# Cryptographic Hash Functions (headers)
  Hacl_Hash_MD5.h
  Hacl_Hash_SHA1.h
  Hacl_Hash_SHA2.h
  Hacl_Hash_SHA3.h
  Hacl_Hash_Blake2b.h
  Hacl_Hash_Blake2s.h
  Hacl_Hash_Blake2b_Simd256.h
  Hacl_Hash_Blake2s_Simd128.h
# Cryptographic Primitives (headers)
  Hacl_HMAC.h
  Hacl_Streaming_HMAC.h
# Cryptographic Hash Functions (internal headers)
  internal/Hacl_Hash_MD5.h
  internal/Hacl_Hash_SHA1.h
  internal/Hacl_Hash_SHA2.h
  internal/Hacl_Hash_SHA3.h
  internal/Hacl_Hash_Blake2b.h
  internal/Hacl_Hash_Blake2s.h
  internal/Hacl_Hash_Blake2b_Simd256.h
  internal/Hacl_Hash_Blake2s_Simd128.h
  internal/Hacl_Impl_Blake2_Constants.h
# Cryptographic Primitives (internal headers)
  internal/Hacl_HMAC.h
  internal/Hacl_Streaming_HMAC.h
# Cryptographic Hash Functions (sources)
  Hacl_Hash_MD5.c
  Hacl_Hash_SHA1.c
  Hacl_Hash_SHA2.c
  Hacl_Hash_SHA3.c
  Hacl_Hash_Blake2b.c
  Hacl_Hash_Blake2s.c
  Hacl_Hash_Blake2b_Simd256.c
  Hacl_Hash_Blake2s_Simd128.c
# Cryptographic Primitives (sources)
  Hacl_HMAC.c
  Hacl_Streaming_HMAC.c
# Miscellaneous
  libintvector.h
  libintvector-shim.h
  lib_memzero0.h
  Lib_Memzero0.c
)

declare -a include_files
include_files=(
  include/krml/lowstar_endianness.h
  include/krml/internal/compat.h
  include/krml/internal/target.h
  include/krml/internal/types.h
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

# Adjust the include path to reflect the local directory structure
$sed -i 's!#include "FStar_UInt128_Verified.h"!#include "krml/FStar_UInt128_Verified.h"!g' include/krml/internal/types.h
$sed -i 's!#include "fstar_uint128_struct_endianness.h"!#include "krml/fstar_uint128_struct_endianness.h"!g' include/krml/internal/types.h

# use KRML_VERIFIED_UINT128
$sed -i -z 's!#define KRML_TYPES_H!#define KRML_TYPES_H\n#define KRML_VERIFIED_UINT128!g' include/krml/internal/types.h

# This contains static inline prototypes that are defined in
# FStar_UInt_8_16_32_64; they are by default repeated for safety of separate
# compilation, but this is not necessary.
$sed -i 's!#include.*Hacl_Krmllib.h"!!g' "${all_files[@]}"

# Use globally unique names for the Hacl_ C APIs to avoid linkage conflicts.
$sed -i -z 's!#include <string.h>!#include <string.h>\n#include "python_hacl_namespaces.h"!' \
  Hacl_Hash_*.h \
  Hacl_HMAC.h \
  Hacl_Streaming_HMAC.h

$sed -i -z 's!#include <inttypes.h>!#include <inttypes.h>\n#include "python_hacl_namespaces.h"!' \
  lib_memzero0.h

# Step 3: trim whitespace (for the linter)

find . -name '*.c' -or -name '*.h' | xargs $sed -i 's![[:space:]]\+$!!'

echo "Updated; verify all is okay using git diff and git status."
