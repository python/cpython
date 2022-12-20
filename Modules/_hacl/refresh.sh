#!/usr/bin/env bash

set -e
set -o pipefail

if [[ $1 == "" ]]; then
  echo "USAGE: $0 path-to-hacl-directory"
  exit 1
fi

hacl_dir=$1
expected_rev=34b8a9fcd91859460b021dabc54deb961e02a675
actual_rev=$(cd "$hacl_dir" && git rev-parse HEAD)

if [[ $actual_rev != $expected_rev ]]; then
  echo "WARNING: HACL* is at revision $actual_rev, but expected revision $expected_rev"
fi

# Step 1: copy files

dist_files="\
  Hacl_Streaming_SHA2.h \
  internal/Hacl_SHA2_Generic.h \
  Hacl_Streaming_SHA2.c"

include_files="\
  include/krml/lowstar_endianness.h \
  include/krml/internal/target.h"

lib_files="\
  krmllib/dist/minimal/FStar_UInt_8_16_32_64.h"

# C files for the algorithms themselves: current directory
(cd $hacl_dir/dist/gcc-compatible && tar cf - $dist_files) | tar xf -

# Support header files (e.g. endianness macros): stays in include/
(cd $hacl_dir/dist/karamel && tar cf - $include_files) | tar xf -

# Special treatment: we don't bother with an extra directory and move krmllib
# files to the same include directory
for f in $lib_files; do
  cp $hacl_dir/dist/karamel/$f include/krml/
done

# Step 2: some in-place modifications to keep things simple and minimal

# This is basic, but refreshes of the vendored HACL code are infrequent, so
# let's not over-engineer this.
if [[ $(uname) == "Darwin" ]]; then
  sed=gsed
else
  sed=sed
fi

all_files=$(find . -name '*.h' -or -name '*.c')

# types.h is a simple wrapper that defines the uint128 type then proceeds to
# include FStar_UInt_8_16_32_64.h; we jump the types.h step since our current
# selection of algorithms does not necessitate the use of uint128
$sed -i 's!#include.*types.h"!#include "krml/FStar_UInt_8_16_32_64.h"!g' $all_files
$sed -i 's!#include.*compat.h"!!g' $all_files

# FStar_UInt_8_16_32_64 contains definitions useful in the general case, but not
# for us; trim!
$sed -i -z 's!\(extern\|typedef\)[^;]*;\n\n!!g' include/krml/FStar_UInt_8_16_32_64.h

# This contains static inline prototypes that are defined in
# FStar_UInt_8_16_32_64; they are by default repeated for safety of separate
# compilation, but this is not necessary.
$sed -i 's!#include.*Hacl_Krmllib.h"!!g' $all_files

# This header is useful for *other* algorithms that refer to SHA2, e.g. Ed25519
# which needs to compute a digest of a message before signing it. Here, since no
# other algorithm builds upon SHA2, this internal header is useless (and is not
# included in $dist_files).
$sed -i 's!#include.*internal/Hacl_Streaming_SHA2.h"!#include "Hacl_Streaming_SHA2.h"!g' $all_files

# The SHA2 file contains all variants of SHA2. We strip 384 and 512 for the time
# being, to be included later.
# This regexp matches a separator (two new lines), followed by:
#
# <non-empty-line>*
# ... 384 or 512 ... {
#   <indented-line>*
# }
#
# The first non-empty lines are the comment block. The second ... may spill over
# the next following lines if the arguments are printed in one-per-line mode.
$sed -i -z 's/\n\n\([^\n]\+\n\)*[^\n]*\(384\|512\)[^{]*{\n\?\(  [^\n]*\n\)*}//g' Hacl_Streaming_SHA2.c

# Same thing with function prototypes
$sed -i -z 's/\n\n\([^\n]\+\n\)*[^\n]*\(384\|512\)[^;]*;//g' Hacl_Streaming_SHA2.h

# Finally, we remove a bunch of ifdefs from target.h that are, again, useful in
# the general case, but not exercised by the subset of HACL* that we vendor.
$sed -z -i 's!#ifndef KRML_\(HOST_PRINTF\|HOST_EXIT\|PRE_ALIGN\|POST_ALIGN\|ALIGNED_MALLOC\|ALIGNED_FREE\|HOST_TIME\)\n\(\n\|#  [^\n]*\n\|[^#][^\n]*\n\)*#endif\n\n!!g' include/krml/internal/target.h
$sed -z -i 's!\n\n\([^#][^\n]*\n\)*#define KRML_\(EABORT\|EXIT\|CHECK_SIZE\)[^\n]*\(\n  [^\n]*\)*!!g' include/krml/internal/target.h
$sed -z -i 's!\n\n\([^#][^\n]*\n\)*#if [^\n]*\n\(  [^\n]*\n\)*#define  KRML_\(EABORT\|EXIT\|CHECK_SIZE\)[^\n]*\(\n  [^\n]*\)*!!g' include/krml/internal/target.h
$sed -z -i 's!\n\n\([^#][^\n]*\n\)*#if [^\n]*\n\(  [^\n]*\n\)*#  define _\?KRML_\(DEPRECATED\|CHECK_SIZE_PRAGMA\|HOST_EPRINTF\|HOST_SNPRINTF\)[^\n]*\n\([^#][^\n]*\n\|#el[^\n]*\n\|#  [^\n]*\n\)*#endif!!g' include/krml/internal/target.h
