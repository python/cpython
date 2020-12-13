#!/bin/bash

# Script to run after a release has been tagged, signed and pushed
# to git.  Will do a fresh checkout, verify the git tag, do fresh
# build/dist, sign the dist with gpg, create a backup copy in HOME,
# upload the tar.gz and sig to sourceware, checkout bzip2-htdocs,
# copy over the new changes, manual, etc. and git push that to update
# https://sourceware.org/bzip2/

# Any error is fatal
set -e

# We take one argument, the version (e.g. 1.0.7)
if [ $# -ne 1 ]; then
  echo "$0 <version> (e.g. 1.0.7)"
  exit 1
fi

VERSION="$1"
echo
echo "  === NOTE ===  "
echo
echo "Requires a sourceware account in the bzip2 group."
echo
echo "Make sure the git repo was tagged, signed and pushed"
echo "If not, please double check the source tree is release ready first"
echo "You probably want to run ./prepare-release.sh $VERSION first."
echo "Then do:"
echo
echo "    git tag -s -m \"bzip2 $VERSION release\" bzip2-$VERSION"
echo "    git push --tags"
echo
read -p "Do you want to continue creating/uploading the release (yes/no)? "

if [ "x$REPLY" != "xyes" ]; then
  echo "OK, till next time."
  exit
fi

echo "OK, creating and updating the release."

# Create a temporary directoy and make sure it is cleaned up.
tempdir=$(mktemp -d) || exit
trap "rm -rf -- ${tempdir}" EXIT

pushd "${tempdir}"

# Checkout
git clone git://sourceware.org/git/bzip2.git
cd bzip2
git tag --verify "bzip2-${VERSION}"
git checkout -b "$VERSION" "bzip2-${VERSION}"

# Create dist (creates bzip2-${VERSION}.tar.gz)
make dist

# Sign (creates bzip2-${VERSION}.tar.gz.sig)
gpg -b bzip2-${VERSION}.tar.gz

# Create backup copy
echo "Putting a backup copy in $HOME/bzip2-$VERSION"
mkdir $HOME/bzip2-$VERSION
cp bzip2-${VERSION}.tar.gz bzip2-${VERSION}.tar.gz.sig $HOME/bzip2-$VERSION/

# Upload
scp bzip2-${VERSION}.tar.gz bzip2-${VERSION}.tar.gz.sig \
    sourceware.org:/sourceware/ftp/pub/bzip2/
ssh sourceware.org "(cd /sourceware/ftp/pub/bzip2 \
  && ln -sf bzip2-$VERSION.tar.gz bzip2-latest.tar.gz \
  && ln -sf bzip2-$VERSION.tar.gz.sig bzip2-latest.tar.gz.sig \
  && ls -lah bzip2-latest*)"

# Update homepage, manual, etc.
cd "${tempdir}"
git clone ssh://sourceware.org/git/bzip2-htdocs.git
cp bzip2/CHANGES bzip2/bzip.css bzip2-htdocs/
cp bzip2/bzip.css bzip2/bzip2.txt bzip2/manual.{html,pdf} bzip2-htdocs/manual/
cd bzip2-htdocs

# Update version in html pages.
sed -i -e "s/The current stable version is bzip2 [0-9]\.[0-9]\.[0-9]\+/The current stable version is bzip2 ${VERSION}/" *.html */*.html

git commit -a -m "Update for bzip2 $VERSION release"
git show
git push

# Cleanup
popd
trap - EXIT
exit
