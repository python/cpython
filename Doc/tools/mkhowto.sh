#! /depot/gnu/plat/bin/bash

MYDIR=`dirname $0`

# DEFAULT_FORMAT must be upper case...
DEFAULT_FORMAT=PDF
USE_DEFAULT_FORMAT=true

# This is needed to support kpathsea based TeX installations.  Others are
# not supported.  ;-)
TEXINPUTS=$MYDIR/../texinputs:
export TEXINPUTS

usage() {
    echo "usage: $0 [options...] file ..."
    exit 2
}

build_html() {
    # This doesn't work; l2hinit.perl uses the current directory, not it's own
    # location.  Need a workaround for this.
    if [ "$ADDRESS" ] ; then
	latex2html -init_file $MYDIR/../perl/l2hinit.perl -address "$ADDRESS" \
	 $1 || exit $?
    else
	latex2html -init_file $MYDIR/../perl/l2hinit.perl $1 || exit $?
    fi
    (cd $FILE; $MYDIR/node2label.pl *.html) || exit $?
}

build_dvi() {
    latex $1 || exit $?
    if [ -f $1.idx ] ; then
	`dirname $0`/fix_hack $1.idx || exit $?
	makeindex $1.idx || exit $?
    fi
    latex $1 || exit $?
}

build_ps() {
    dvips -N0 -f $1 >$1.ps || exit $?
}

build_pdf() {
    # We really have to do it three times to get all the page numbers right,
    # since the length of the ToC makes a real difference.
    pdflatex $1 || exit $?
    pdflatex $1 || exit $?
    `dirname $0`/toc2bkm.py -c section $FILE || exit $?
    if [ -f $1.idx ] ; then
	`dirname $0`/fix_hack $1.idx || exit $?
	makeindex $1.idx || exit $?
    fi
    pdflatex $1 || exit $?
}

# figure out what our targets are:
while [ "$1" ] ; do
    case "$1" in
	--pdf|--pd)
	    BUILD_PDF=true
	    USE_DEFAULT_FORMAT=false
	    shift 1
	    ;;
	--ps)
	    BUILD_PS=true
	    USE_DEFAULT_FORMAT=false
	    shift 1
	    ;;
	--dvi|--dv|--d)
	    BUILD_DVI=true
	    USE_DEFAULT_FORMAT=false
	    shift 1
	    ;;
	--html|--htm|--ht|--h)
	    BUILD_HTML=true
	    USE_DEFAULT_FORMAT=false
	    shift 1
	    ;;
	-a|--address|--addres|--addre|-addr|--add|--ad|--a)
	    ADDRESS="$2"
	    shift 2
	    ;;
	-*)
	    usage
	    ;;
	*)
	    break;;
    esac
done

if [ $# = 0 ] ; then
    usage
fi

if [ $USE_DEFAULT_FORMAT = true ] ; then
    eval "BUILD_$DEFAULT_FORMAT=true"
fi

for FILE in $@ ; do
    FILE=${FILE%.tex}
    if [ "$BUILD_DVI" -o "$BUILD_PS" ] ; then
	build_dvi $FILE
    fi
    if [ "$BUILD_PDF" ] ; then
	build_pdf $FILE
    fi
    if [ "$BUILD_PS" ] ; then
	build_ps $FILE
    fi
    if [ "$BUILD_HTML" ] ; then
	if [ ! "$BUILD_DVI" -o ! "$BUILD_PDF" ] ; then
	    # need to get aux file
	    build_dvi $FILE
	fi
	build_html $FILE
    fi
    rm -f $FILE.aux $FILE.log $FILE.out $FILE.toc $FILE.bkm
    if [ ! "$BUILD_DVI" ] ; then
	rm -f $FILE.dvi
    fi
done
