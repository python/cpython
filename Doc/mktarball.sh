#! /bin/sh
#
#  script to create the latex source distribution
#  * should be modified to get the Python version number automatically
#    from the Makefile or someplace.
#
#  usage:
#	./mktarball.sh [tag]
#
#  without [tag]:  generate from the current version that's checked in
#		   (*NOT* what's in the current directory!)
#
#  with [tag]:  generate from the named tag

VERSION=1.5

TEMPDIR=tmp.$$
MYDIR=`pwd`

TAG="$1"

mkdirhier $TEMPDIR/Doc || exit $?
if [ "$TAG" ] ; then
    cvs export -r $TAG -d $TEMPDIR/Doc python/dist/src/Doc || exit $?
else
    cvs checkout -d $TEMPDIR/Doc python/dist/src/Doc || exit $?
    rm -r `find $TEMPDIR -name CVS -print` || exit $?
fi

cd $TEMPDIR

(tar cf - Doc | gzip -9 >$MYDIR/latex-$VERSION.tar.gz) || exit $?
cd $MYDIR
rm -r $TEMPDIR || exit $?

exit 0
