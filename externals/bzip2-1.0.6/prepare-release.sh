#!/bin/bash

# Script to run to prepare a new release.
# It will update the release number and tell you to update the
# CHANGES file and to double check everything looks before doing
# the release commit and tagging.

# Afterwards you probably want to run release-update.sh to upload
# the release and update the website at https://sourceware.org/bzip2/

# Any error is fatal
set -e

# We take one argument, the version (e.g. 1.0.7)
if [ $# -ne 1 ]; then
  echo "$0 <version> (e.g. 1.0.7)"
  exit 1
fi

LANG=C
VERSION="$1"
DATE=$(date +"%d %B %Y")
DAY=$(date +"%d")
MONTH=$(date +"%B")
SHORTMONTH=$(date +"%b")
YEAR=$(date +"%Y")

# Replace the version strings and date ranges in the comments
VER_PREFIX="bzip2/libbzip2 version "
sed -i -e "s@${VER_PREFIX}[0-9].*@${VER_PREFIX}${VERSION} of ${DATE}@" \
       -e "s@ (C) \([0-9]\+\)-[0-9]\+ @ (C) \1-$YEAR @" \
  CHANGES LICENSE Makefile* README* *.c *.h *.pl *.sh

# Add an entry to the README
printf "%2s %8s %s\n" "$DAY" "$MONTH" "$YEAR (bzip2, version $VERSION)" \
  >> README

# Update manual
sed -i -e "s@ENTITY bz-version \".*\"@ENTITY bz-version \"$VERSION\"@" \
       -e "s@ENTITY bz-date \".*\"@ENTITY bz-date \"$DAY $MONTH $YEAR\"@" \
       -e "s@ENTITY bz-lifespan \"\([0-9]\+\)-[0-9]\+\"@ENTITY bz-lifespan \"\1-$YEAR\"@"\
  entities.xml

# bzip2.1 should really be generated from the manual.xml, but currently
# isn't, so explicitly change it here too.
sed -i -e "s@This manual page pertains to version .* of@This manual page pertains to version $VERSION of@" \
       -e "s@sorting file compressor, v.*@sorting file compressor, v$VERSION@" \
  bzip2.1

# Update sources. All sources, use bzlib_private.
# Except bzip2recover, which embeds a version string...
sed -i -e "s@^#define BZ_VERSION  \".*\"@#define BZ_VERSION  \"${VERSION}, ${DAY}-${SHORTMONTH}-${YEAR}\"@" \
  bzlib_private.h
sed -i -e "s@\"bzip2recover .*: extracts blocks from damaged@\"bzip2recover ${VERSION}: extracts blocks from damaged@" \
  bzip2recover.c

# And finally update the version/dist/so_name in the Makefiles.
sed -i -e "s@^DISTNAME=bzip2-.*@DISTNAME=bzip2-${VERSION}@" \
  Makefile
sed -i -e "s@libbz2\.so\.[0-9]\.[0-9]\.[0-9]*@libbz2\.so\.${VERSION}@" \
  Makefile-libbz2_so

echo "Now make sure the diff looks correct:"
echo "  git diff"
echo
echo "And make sure there is a $VERSION section in the CHANGES file."
echo
echo "Double check:"
echo "  make clean && make dist && make clean && make -f Makefile-libbz2_so"
echo
echo "Does everything look fine?"
echo
echo "git commit -a -m \"Prepare for $VERSION release.\""
echo "git push"
echo
echo "Wait for the buildbot to give the all green!"
echo "Then..."
echo
echo "git tag -s -m \"bzip2 $VERSION release\" bzip2-$VERSION"
echo "git push --tags"
echo
echo "./release-update.sh"
