#!/bin/sh
#
# ===========================================================================
# FILE:         makexp_aix
# TYPE:         standalone executable
# SYSTEM:	    AIX
#
# DESCRIPTION:  This script creates an export list of ALL global symbols
#               from a list of object or archive files.
#
# USAGE:        makexp_aix <OutputFile> "<FirstLine>" <InputFile> ...
#
#               where:
#                      <OutputFile> is the target export list filename.
#                      <FirstLine> is the path/file string to be appended
#                         after the "#!" symbols in the first line of the
#                         export file. Passing "" means deferred resolution.
#                      <InputFile> is an object (.o) or an archive file (.a).
#
# HISTORY:
#		3-Apr-1998  -- remove C++ entries of the form Class::method
#		Vladimir Marangozov
#
#               1-Jul-1996  -- added header information
#               Vladimir Marangozov
#
#               28-Jun-1996 -- initial code
#               Vladimir Marangozov           (Vladimir.Marangozov@imag.fr)
# ==========================================================================

# Variables
expFileName=$1
toAppendStr=$2
shift; shift;
inputFiles=$*
automsg="Generated automatically by makexp_aix"
notemsg="NOTE: lists _all_ global symbols defined in the above file(s)."
curwdir=`pwd`

# Create the export file and setup the header info
echo "#!"$toAppendStr > $expFileName
echo "*" >> $expFileName
echo "* $automsg  (`date -u`)" >> $expFileName
echo "*" >> $expFileName
echo "* Base Directory: $curwdir" >> $expFileName
echo "* Input File(s) : $inputFiles" >> $expFileName
echo "*" >> $expFileName
echo "* $notemsg" >> $expFileName
echo "*" >> $expFileName

# Extract the symbol list using 'nm'
# Here are some hidden tricks:
# - Use the -B flag to have a standard BSD representation
#   of the symbol list.
# - Use the -x flag to have a hex representation of the symbol
#   values. This fills the leading whitespaces, thus simplifying
#   the sed statement.
# - Eliminate all entries except those with either "B", "D"
#   or "T" key letters. We are interested only in the global
#   (extern) BSS, DATA and TEXT symbols. With the same statement
#   we eliminate object member lines relevant to AIX 4.
# - Eliminate entries containing a dot. We can have a dot only
#   as a symbol prefix, but such symbols are undefined externs.
# - Eliminate everything including the key letter, so that we're
#   left with just the symbol name.
# - Eliminate all entries containing two colons, like Class::method
#

/usr/ccs/bin/nm -Bex -X32_64 $inputFiles \
| sed -e '/ [^BDT] /d' -e '/\./d' -e 's/.* [BDT] //' -e '/::/d'	\
| sort | uniq >> $expFileName
