#!/bin/bash
#
#############################################################################
#
# 7z2lzma.bash is very primitive .7z to .lzma converter. The input file must
# have exactly one LZMA compressed stream, which has been created with the
# default lc, lp, and pb values. The CRC32 in the .7z archive is not checked,
# and the script may seem to succeed while it actually created a corrupt .lzma
# file. You should always try uncompressing both the original .7z and the
# created .lzma and compare that the output is identical.
#
# This script requires basic GNU tools and 7z or 7za tool from p7zip.
#
# Last modified: 2009-01-15 14:25+0200
#
#############################################################################
#
# Author: Lasse Collin <lasse.collin@tukaani.org>
#
# This file has been put into the public domain.
# You can do whatever you want with this file.
#
#############################################################################

# You can use 7z or 7za, both will work.
SEVENZIP=7za

if [ $# != 2 -o -z "$1" -o -z "$2" ]; then
	echo "Usage: $0 input.7z output.lzma"
	exit 1
fi

# Converts an integer variable to little endian binary integer.
int2bin()
{
	local LEN=$1
	local NUM=$2
	local HEX=(0 1 2 3 4 5 6 7 8 9 A B C D E F)
	local I
	for ((I=0; I < "$LEN"; ++I)); do
		printf "\\x${HEX[(NUM >> 4) & 0x0F]}${HEX[NUM & 0x0F]}"
		NUM=$((NUM >> 8))
	done
}

# Make sure we get possible errors from pipes.
set -o pipefail

# Get information about the input file. At least older 7z and 7za versions
# may return with zero exit status even when an error occurred, so check
# if the output has any lines beginning with "Error".
INFO=$("$SEVENZIP" l -slt "$1")
if [ $? != 0 ] || printf '%s\n' "$INFO" | grep -q ^Error; then
	printf '%s\n' "$INFO"
	exit 1
fi

# Check if the input file has more than one compressed block.
if printf '%s\n' "$INFO" | grep -q '^Block = 1'; then
	echo "Cannot convert, because the input file has more than"
	echo "one compressed block."
	exit 1
fi

# Get compressed, uncompressed, and dictionary size.
CSIZE=$(printf '%s\n' "$INFO" | sed -rn 's|^Packed Size = ([0-9]+$)|\1|p')
USIZE=$(printf '%s\n' "$INFO" | sed -rn 's|^Size = ([0-9]+$)|\1|p')
DICT=$(printf '%s\n' "$INFO" | sed -rn 's|^Method = LZMA:([0-9]+[bkm]?)$|\1|p')

if [ -z "$CSIZE" -o -z "$USIZE" -o -z "$DICT" ]; then
	echo "Parsing output of $SEVENZIP failed. Maybe the file uses some"
	echo "other compression method than plain LZMA."
	exit 1
fi

# The following assumes that the default lc, lp, and pb settings were used.
# Otherwise the output will be corrupt.
printf '\x5D' > "$2"

# Dictionary size can be either was power of two, bytes, kibibytes, or
# mebibytes. We need to convert it to bytes.
case $DICT in
	*b)
		DICT=${DICT%b}
		;;
	*k)
		DICT=${DICT%k}
		DICT=$((DICT << 10))
		;;
	*m)
		DICT=${DICT%m}
		DICT=$((DICT << 20))
		;;
	*)
		DICT=$((1 << DICT))
		;;
esac
int2bin 4 "$DICT" >> "$2"

# Uncompressed size
int2bin 8 "$USIZE" >> "$2"

# Copy the actual compressed data. Using multiple dd commands to avoid
# copying large amount of data with one-byte block size, which would be
# annoyingly slow.
BS=8192
BIGSIZE=$((CSIZE / BS))
CSIZE=$((CSIZE % BS))
{
	dd of=/dev/null bs=32 count=1 \
		&& dd bs="$BS" count="$BIGSIZE" \
		&& dd bs=1 count="$CSIZE"
} < "$1" >> "$2"

exit $?
