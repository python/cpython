#!/usr/bin/env bash
#
# Use this script to update libexpat

set -e
set -o pipefail

if [[ "${BASH_VERSINFO[0]}" -lt 4 ]]; then
  echo "A bash version >= 4 required. Got: $BASH_VERSION" >&2
  exit 1
fi

# Update this when updating to a new version after verifying that the changes
# the update brings in are good. These values are used for verifying the SBOM, too.
expected_libexpat_tag="R_2_7_3"
expected_libexpat_version="2.7.3"
expected_libexpat_sha256="821ac9710d2c073eaf13e1b1895a9c9aa66c1157a99635c639fbff65cdbdd732"

expat_dir="$(realpath "$(dirname -- "${BASH_SOURCE[0]}")")"
cd ${expat_dir}

# Step 1: download and copy files
curl --location "https://github.com/libexpat/libexpat/releases/download/${expected_libexpat_tag}/expat-${expected_libexpat_version}.tar.gz" > libexpat.tar.gz
echo "${expected_libexpat_sha256} libexpat.tar.gz" | sha256sum --check

# Step 2: Pull files from the libexpat distribution
declare -a lib_files
lib_files=(
  ascii.h
  asciitab.h
  expat.h
  expat_external.h
  iasciitab.h
  internal.h
  latin1tab.h
  nametab.h
  siphash.h
  utf8tab.h
  winconfig.h
  xmlparse.c
  xmlrole.c
  xmlrole.h
  xmltok.c
  xmltok.h
  xmltok_impl.c
  xmltok_impl.h
  xmltok_ns.c
)
for f in "${lib_files[@]}"; do
  tar xzvf libexpat.tar.gz "expat-${expected_libexpat_version}/lib/${f}" --strip-components 2
done
rm libexpat.tar.gz

# Step 3: Add the namespacing include to expat_external.h
sed -i 's/#  define Expat_External_INCLUDED 1/&\n\/* Namespace external symbols to allow multiple libexpat version to\n   co-exist. \*\/\n#include "pyexpatns.h"/' expat_external.h

if ! grep -q '#include "pyexpatns\.h"' expat_external.h; then
  echo "
Error: namespacing include not found in expat_external.h;
This may be due to source changes and will require updating this script" >&2
  exit 1
fi

echo "
Updated! next steps:
- Verify all is okay:
    git diff
    git status
- Regenerate the sbom file
    make regen-sbom
- Update warning count in Tools/build/.warningignore_macos
    (use info from CI if not on a Mac)
"
