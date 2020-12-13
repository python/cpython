#!/bin/bash
#
###############################################################################
#
# Build a binary package on Windows with MinGW and MSYS
#
# Set the paths where MinGW, Mingw-w32, or MinGW-w64 are installed. If both
# MinGW and MinGW-w32 are specified, MinGW-w32 will be used. If there is no
# 32-bit or 64-bit compiler at all, it is simply skipped.
#
# Optionally, 7-Zip is used to create the final .zip and .7z packages.
# If you have installed it in the default directory, this script should
# find it automatically. Otherwise adjust the path manually.
#
# If you want to use a cross-compiler e.g. on GNU/Linux, this script won't
# work out of the box. You need to omit "make check" commands and replace
# u2d with some other tool to convert newlines from LF to CR+LF. You will
# also need to pass the --host option to configure.
#
###############################################################################
#
# Author: Lasse Collin
#
# This file has been put into the public domain.
# You can do whatever you want with this file.
#
###############################################################################

MINGW_DIR=/c/devel/tools/mingw
MINGW_W32_DIR=/c/devel/tools/mingw-w32
MINGW_W64_DIR=/c/devel/tools/mingw-w64

for SEVENZ_EXE in "$PROGRAMW6432/7-Zip/7z.exe" "$PROGRAMFILES/7-Zip/7z.exe" \
		"/c/Program Files/7-Zip/7z.exe"
do
	[ -x "$SEVENZ_EXE" ] && break
done


# Abort immediately if something goes wrong.
set -e

# White spaces in directory names may break things so catch them immediately.
case $(pwd) in
	' ' | '	' | '
') echo "Error: White space in the directory name" >&2; exit 1 ;;
esac

# This script can be run either at the top-level directory of the package
# or in the same directory containing this script.
if [ ! -f windows/build.bash ]; then
	cd ..
	if [ ! -f windows/build.bash ]; then
		echo "You are in a wrong directory." >&2
		exit 1
	fi
fi

# Run configure and copy the binaries to the given directory.
#
# The first argument is the directory where to copy the binaries.
# The rest of the arguments are passed to configure.
buildit()
{
	DESTDIR=$1
	BUILD=$2
	CFLAGS=$3

	# Clean up if it was already configured.
	[ -f Makefile ] && make distclean

	# Build the size-optimized binaries. Providing size-optimized liblzma
	# could be considered but I don't know if it should only use -Os or
	# should it also use --enable-small and if it should support
	# threading. So I don't include a size-optimized liblzma for now.
	./configure \
		--prefix= \
		--enable-silent-rules \
		--disable-dependency-tracking \
		--disable-nls \
		--disable-scripts \
		--disable-threads \
		--disable-shared \
		--enable-small \
		--build="$BUILD" \
		CFLAGS="$CFLAGS -Os"
	make check

	mkdir -pv "$DESTDIR"
	cp -v src/xzdec/{xz,lzma}dec.exe src/lzmainfo/lzmainfo.exe "$DESTDIR"

	make distclean

	# Build the normal speed-optimized binaries. The type of threading
	# (win95 vs. vista) will be autodetect from the target architecture.
	./configure \
		--prefix= \
		--enable-silent-rules \
		--disable-dependency-tracking \
		--disable-nls \
		--disable-scripts \
		--build="$BUILD" \
		CFLAGS="$CFLAGS -O2"
	make -C src/liblzma
	make -C src/xz LDFLAGS=-static
	make -C tests check

	cp -v src/xz/xz.exe src/liblzma/.libs/liblzma.a "$DESTDIR"
	cp -v src/liblzma/.libs/liblzma-*.dll "$DESTDIR/liblzma.dll"

	strip -v "$DESTDIR/"*.{exe,dll}
	strip -vg "$DESTDIR/"*.a
}

# Copy files and convert newlines from LF to CR+LF. Optionally add a suffix
# to the destination filename.
#
# The first argument is the destination directory. The second argument is
# the suffix to append to the filenames; use empty string if no extra suffix
# is wanted. The rest of the arguments are actual the filenames.
txtcp()
{
	DESTDIR=$1
	SUFFIX=$2
	shift 2
	for SRCFILE; do
		DESTFILE="$DESTDIR/${SRCFILE##*/}$SUFFIX"
		echo "Converting \`$SRCFILE' -> \`$DESTFILE'"
		u2d < "$SRCFILE" > "$DESTFILE"
	done
}

if [ -d "$MINGW_W32_DIR" ]; then
	# 32-bit x86, Win95 or later, using MinGW-w32
	PATH=$MINGW_W32_DIR/bin:$MINGW_W32_DIR/i686-w64-mingw32/bin:$PATH \
			buildit \
			pkg/bin_i686 \
			i686-w64-mingw32 \
			'-march=i686 -mtune=generic'
	# 32-bit x86 with SSE2, Win98 or later, using MinGW-w32
	PATH=$MINGW_W32_DIR/bin:$MINGW_W32_DIR/i686-w64-mingw32/bin:$PATH \
			buildit \
			pkg/bin_i686-sse2 \
			i686-w64-mingw32 \
			'-march=i686 -msse2 -mfpmath=sse -mtune=generic'
elif [ -d "$MINGW_DIR" ]; then
	# 32-bit x86, Win95 or later, using MinGW
	PATH=$MINGW_DIR/bin:$PATH \
			buildit \
			pkg/bin_i486 \
			i486-pc-mingw32 \
			'-march=i486 -mtune=generic'
fi

if [ -d "$MINGW_W64_DIR" ]; then
	# x86-64, Windows Vista or later, using MinGW-w64
	PATH=$MINGW_W64_DIR/bin:$MINGW_W64_DIR/x86_64-w64-mingw32/bin:$PATH \
			buildit \
			pkg/bin_x86-64 \
			x86_64-w64-mingw32 \
			'-march=x86-64 -mtune=generic'
fi

# Copy the headers, the .def file, and the docs.
# They are the same for all architectures and builds.
mkdir -pv pkg/{include/lzma,doc/{manuals,examples}}
txtcp pkg/include "" src/liblzma/api/lzma.h
txtcp pkg/include/lzma "" src/liblzma/api/lzma/*.h
txtcp pkg/doc "" src/liblzma/liblzma.def
txtcp pkg/doc .txt AUTHORS COPYING NEWS README THANKS TODO
txtcp pkg/doc "" doc/*.txt windows/README-Windows.txt
txtcp pkg/doc/manuals "" doc/man/txt/{xz,xzdec,lzmainfo}.txt
cp -v doc/man/pdf-*/{xz,xzdec,lzmainfo}-*.pdf pkg/doc/manuals
txtcp pkg/doc/examples "" doc/examples/*

if [ -f windows/COPYING-Windows.txt ]; then
	txtcp pkg/doc "" windows/COPYING-Windows.txt
fi

# Create the package. This requires 7z.exe from 7-Zip. If it wasn't found,
# this step is skipped and you have to zip it yourself.
VER=$(sh build-aux/version.sh)
cd pkg
if [ -x "$SEVENZ_EXE" ]; then
	"$SEVENZ_EXE" a -tzip ../xz-$VER-windows.zip *
	"$SEVENZ_EXE" a ../xz-$VER-windows.7z *
else
	echo
	echo "NOTE: 7z.exe was not found. xz-$VER-windows.zip"
	echo "      and xz-$VER-windows.7z were not created."
	echo "      You can create them yourself from the pkg directory."
fi

if [ ! -f ../windows/COPYING-Windows.txt ]; then
	echo
	echo "NOTE: windows/COPYING-Windows.txt doesn't exists."
	echo "      MinGW(-w64) runtime copyright information"
	echo "      is not included in the package."
fi

echo
echo "Build completed successfully."
echo
