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

if [ "$1" = "-t" -o "$1" = "--tools" ] ; then
    shift 1
    TOOLS_ONLY=true
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
    (tar cf - Doc | gzip -9 >$MYDIR/tools-$RELEASE.tgz) || exit $?
else
    cd $TEMPDIR

    (tar cf - Python-$RELEASE | gzip -9 >$MYDIR/latex-$RELEASE.tgz) || exit $?
fi
cd $MYDIR
rm -r $TEMPDIR || exit $?

exit 0
