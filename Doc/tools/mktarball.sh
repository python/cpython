#! /bin/sh
#
#  script to create the latex source distribution
#  * should be modified to get the Python version number automatically
#    from the Makefile or someplace.
#
#  usage:
#	./mktarball.sh [-t|--tools] release [tag]
#
#  with -t|--tools:  doesn't include the documents, only the framework
#
#  without [tag]:  generate from the current version that's checked in
#		   (*NOT* what's in the current directory!)
#
#  with [tag]:  generate from the named tag

#  VERSION='$Revision$'

GZIP='gzip -9'
GZIPEXT=tgz

if [ "$1" = "-t" -o "$1" = "--tools" ] ; then
    shift 1
    TOOLS_ONLY=true
fi
if [ "$1" = "-z" -o "$1" = "--zip" ] ; then
    shift 1
    USE_ZIP=true
fi
if [ "$1" = "-g" -o "$1" = "--targz" -o "$1" = "--tgz" ] ; then
    shift 1
    USE_ZIP=''
fi
if [ "$1" = "-b" -o "$1" = "--bz2" -o "$1" = "--bzip2" ] ; then
    shift 1
    GZIP='bzip2 -9'
    GZIPEXT=tar.bz2
fi

RELEASE=$1; shift

TEMPDIR=tmp.$$
MYDIR=`pwd`

TAG="$1"

mkdirhier $TEMPDIR/Python-$RELEASE/Doc || exit $?
if [ "$TAG" ] ; then
    cvs export -r $TAG -d $TEMPDIR/Python-$RELEASE/Doc python/dist/src/Doc \
     || exit $?
else
    cvs checkout -d $TEMPDIR/Python-$RELEASE/Doc python/dist/src/Doc || exit $?
    rm -r `find $TEMPDIR -name CVS -print` || exit $?
fi

rm -f `find $TEMPDIR -name .cvsignore -print`

rm -f $TEMPDIR/Python-$RELEASE/Doc/ref/ref.pdf
rm -f $TEMPDIR/Python-$RELEASE/Doc/ref/ref.ps


if [ "$TOOLS_ONLY" ] ; then
    cd $TEMPDIR/Python-$RELEASE/Doc
    # remove the actual documents
    rm -rf api ext lib mac ref tut
    cd ..
    if [ "$USE_ZIP" ] ; then
	zip -r9 tools-$RELEASE.zip Doc || exit
    else
	(tar cf - Doc | $GZIP >$MYDIR/tools-$RELEASE.$GZIPEXT) || exit $?
    fi
else
    cd $TEMPDIR
    if [ "$USE_ZIP" ] ; then
	zip -r9 $MYDIR/latex-$RELEASE.zip Python-$RELEASE || exit $?
    else
	(tar cf - Python-$RELEASE | $GZIP >$MYDIR/latex-$RELEASE.$GZIPEXT) \
	 || exit $?
    fi
fi
cd $MYDIR
rm -r $TEMPDIR || exit $?

exit 0
