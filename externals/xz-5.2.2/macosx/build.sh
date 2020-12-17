#!/bin/sh

###############################################################################
# Author: Anders F Björklund <afb@users.sourceforge.net>
#
# This file has been put into the public domain.
# You can do whatever you want with this file.
###############################################################################

mkdir -p Root
mkdir -p Resources

# Abort immediately if something goes wrong.
set -e

GCC="gcc-4.2"
SDK="/Developer/SDKs/MacOSX10.5.sdk"
MDT="10.5"
GTT=i686-apple-darwin9

ARCHES1="-arch ppc -arch ppc64 -arch i386 -arch x86_64"
ARCHES2="-arch ppc -arch i386"
PKGFORMAT="10.5" # xar

# avoid "unknown required load command: 0x80000022" from linking on Snow Leopard
uname -r | grep ^1 >/dev/null && LDFLAGS="$LDFLAGS -Wl,-no_compact_linkedit"

# Clean up if it was already configured.
[ -f Makefile ] && make distclean

# Build the regular fat program

CC="$GCC" \
CFLAGS="-O2 -g $ARCHES1 -isysroot $SDK -mmacosx-version-min=$MDT" \
../configure --disable-dependency-tracking --disable-xzdec --disable-lzmadec $GTT

make

make check

make DESTDIR=`pwd`/Root install

make distclean

# Build the size-optimized program

CC="$GCC" \
CFLAGS="-Os -g $ARCHES2 -isysroot $SDK -mmacosx-version-min=$MDT" \
../configure --disable-dependency-tracking --disable-shared --disable-nls --disable-encoders --enable-small --disable-threads $GTT

make -C src/liblzma
make -C src/xzdec
make -C src/xzdec DESTDIR=`pwd`/Root install

cp -a ../extra Root/usr/local/share/doc/xz

make distclean

# Move development files to different package

test -d liblzma && rm -r liblzma
mkdir -p liblzma/usr/local

mv Root/usr/local/include liblzma/usr/local
mv Root/usr/local/lib liblzma/usr/local

mkdir -p Root/usr/local/lib
cp -p liblzma/usr/local/lib/liblzma.5.dylib Root/usr/local/lib
mkdir -p liblzma/usr/local/share/doc/xz
mv Root/usr/local/share/doc/xz/examples* liblzma/usr/local/share/doc/xz

# Strip debugging symbols and make relocatable

for bin in xz lzmainfo xzdec lzmadec; do
    strip -S Root/usr/local/bin/$bin
    install_name_tool -change /usr/local/lib/liblzma.5.dylib @executable_path/../lib/liblzma.5.dylib Root/usr/local/bin/$bin
done

for lib in liblzma.5.dylib; do
    strip -S Root/usr/local/lib/$lib
    install_name_tool -id @executable_path/../lib/liblzma.5.dylib Root/usr/local/lib/$lib
done

# Create tarball, but without the HFS+ attrib

rmdir debug lib po src/liblzma/api src/liblzma src/lzmainfo src/scripts src/xz src/xzdec src tests

( cd Root/usr/local; COPY_EXTENDED_ATTRIBUTES_DISABLE=true COPYFILE_DISABLE=true tar cvjf ../../../XZ.tbz * )
( cd liblzma; COPY_EXTENDED_ATTRIBUTES_DISABLE=true COPYFILE_DISABLE=true tar cvjf ../liblzma.tbz ./usr/local )

# Include documentation files for package

cp -p ../README Resources/ReadMe.txt
cp -p ../COPYING Resources/License.txt

# Make an Installer.app package

ID="org.tukaani.xz"
VERSION=`cd ..; sh build-aux/version.sh`
PACKAGEMAKER=/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker
$PACKAGEMAKER -r Root/usr/local -l /usr/local -e Resources -i $ID -n $VERSION -t XZ -o XZ.pkg -g $PKGFORMAT --verbose
$PACKAGEMAKER -r liblzma -w -k -i $ID.liblzma -n $VERSION -o liblzma.pkg -g $PKGFORMAT --verbose

# Put the package in a disk image

if [ "$PKGFORMAT" != "10.5" ]; then
hdiutil create -fs HFS+ -format UDZO -quiet -srcfolder XZ.pkg -ov XZ.dmg
hdiutil internet-enable -yes -quiet XZ.dmg
fi

echo
echo "Build completed successfully."
echo
