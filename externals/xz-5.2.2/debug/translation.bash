#!/bin/bash

###############################################################################
#
# Script to check output of some translated messages
#
# This should be useful for translators to check that the translated strings
# look good. This doesn't make xz print all possible strings, but it should
# cover most of the cases where mistakes can easily happen.
#
# Give the path and filename of the xz executable as an argument. If no
# arguments are given, this script uses ../src/xz/xz (relative to the
# location of this script).
#
# You may want to pipe the output of this script to less -S to view the
# tables printed by xz --list on a 80-column terminal. On the other hand,
# viewing the other messages may be better without -S.
#
###############################################################################
#
# Author: Lasse Collin
#
# This file has been put into the public domain.
# You can do whatever you want with this file.
#
###############################################################################

set -e

# If an argument was given, use it to set the location of the xz executable.
unset XZ
if [ -n "$1" ]; then
	XZ=$1
	[ "x${XZ:0:1}" != "x/" ] && XZ="$PWD/$XZ"
fi

# Locate top_srcdir and go there.
top_srcdir="$(cd -- "$(dirname -- "$0")" && cd .. && pwd)"
cd -- "$top_srcdir"

# If XZ wasn't already set, use the default location.
XZ=${XZ-"$PWD/src/xz/xz"}
if [ "$(type -t "$XZ" || true)" != "file" ]; then
	echo "Give the location of the xz executable as an argument" \
			"to this script."
	exit 1
fi
XZ=$(type -p -- "$XZ")

# Print the xz version and locale information.
echo "$XZ --version"
"$XZ" --version
echo
if [ -d .git ] && type git > /dev/null 2>&1; then
	echo "Source code version in $PWD:"
	git describe --abbrev=4
fi
echo
locale
echo

# Make the test files directory the current directory.
cd tests/files

# Put xz in PATH so that argv[0] stays short.
PATH=${XZ%/*}:$PATH

# Some of the test commands are error messages and thus don't
# return successfully.
set +e

for CMD in \
	"xz --foobarbaz" \
	"xz --memlimit=123abcd" \
	"xz --memlimit=40MiB -6 /dev/null" \
	"xz --memlimit=0 --info-memory" \
	"xz --memlimit-compress=1234MiB --memlimit-decompress=50MiB --info-memory" \
	"xz --verbose --verbose /dev/null | cat" \
	"xz --lzma2=foobarbaz" \
	"xz --lzma2=foobarbaz=abcd" \
	"xz --lzma2=mf=abcd" \
	"xz --lzma2=preset=foobarbaz" \
	"xz --lzma2=mf=bt4,nice=2" \
	"xz --lzma2=nice=50000" \
	"xz --help" \
	"xz --long-help" \
	"xz --list good-*lzma2*" \
	"xz --list good-1-check*" \
	"xz --list --verbose good-*lzma2*" \
	"xz --list --verbose good-1-check*" \
	"xz --list --verbose --verbose good-*lzma2*" \
	"xz --list --verbose --verbose good-1-check*" \
	"xz --list --verbose --verbose unsupported-check.xz"
do
	echo "-----------------------------------------------------------"
	echo
	echo "\$ $CMD"
	eval "$CMD"
	echo
done 2>&1
