#! /depot/gnu/plat/bin/bash

MYDIR=`dirname $0`
WORKDIR=`pwd`
cd $MYDIR
MYDIR=`pwd`
cd ..
TOPDIR=`pwd`
cd $WORKDIR

# DEFAULT_FORMAT must be upper case...
DEFAULT_FORMAT=PDF
USE_DEFAULT_FORMAT=true
DISCARD_TEMPS=true

L2H_INIT_FILE=$TOPDIR/perl/l2hinit.perl

LOGFILE=/usr/tmp/mkhowto-$LOGNAME-$$.how
LOGGING=''

usage() {
    MYNAME=`basename $0`
    echo "usage: $MYNAME [options...] file ..."
    cat <<EOF

Options specifying formats to build:
    --html		HyperText Markup Language
    --pdf		Portable Document Format (default)
    --ps		PostScript
    --dvi		"DeVice Indepentent" format from TeX

    More than one output format may be specified.

HTML options:
    --address, -a	Specify an address for page footers.
    --link		Specify the number of levels to include on each page.
    --split, -s		Specify a section level for page splitting.

Other options:
    --a4		Format for A4 paper.
    --letter		Format for US letter paper (the default).
    --help, -H		Show this text.
    --logging, -l	Log stdout and stderr to a file (*.how).
    --debugging, -D	Echo commands as they are executed.
    --keep, -k		Keep temporary files around.
    --quiet, -q		Do not print command output to stdout.
			(stderr is also lost,  sorry; see *.how for errors)

EOF
    
    exit $1
}

# These are LaTeX2HTML controls; they reflect l2h variables of the same name.
# The values here are the defaults after modification by perl/l2hinit.perl.
#
ADDRESS=''
MAX_LINK_DEPTH=3
MAX_SPLIT_DEPTH=8

build_html() {
    TEXFILE=`kpsewhich $1.tex`
    if [ "$ADDRESS" ] ; then
	latex2html -init_file $L2H_INIT_FILE \
	 -address "$ADDRESS" \
	 -link $MAX_LINK_DEPTH -split $MAX_SPLIT_DEPTH \
	 $1 || exit $?
    else
	latex2html -init_file $L2H_INIT_FILE \
	 -link $MAX_LINK_DEPTH -split $MAX_SPLIT_DEPTH \
	 -dir $1 $TEXFILE || exit $?
    fi
    if [ "$MAX_SPLIT_DEPTH" -ne 1 ] ; then
	(cd $1; $MYDIR/node2label.pl *.html) || exit $?
    fi
}

use_latex() {
    # two args:  <file> <latextype>
    MYFILE=$1
    MYLATEX=$2
    #
    # We really have to do it three times to get all the page numbers right,
    # since the length of the ToC makes a real difference.
    #
    $MYDIR/newind.py >$MYFILE.ind
    $MYDIR/newind.py modindex >mod$MYFILE.ind
    $MYLATEX $MYFILE || exit $?
    if [ -f mod$MYFILE.idx ] ; then
	makeindex mod$MYFILE.idx
    fi
    if [ -f $MYFILE.idx ] ; then
	$MYDIR/fix_hack $MYFILE.idx
	makeindex $MYFILE.idx
	$MYDIR/indfix.py $MYFILE.ind
    fi
    $MYLATEX $MYFILE || exit $?
    if [ -f mod$MYFILE.idx ] ; then
	makeindex mod$MYFILE.idx
    fi
    if [ -f $MYFILE.idx ] ; then
	$MYDIR/fix_hack $MYFILE.idx || exit $?
	makeindex -s $TOPDIR/texinputs/myindex.ist $MYFILE.idx || exit $?
    fi
    if [ -f $MYFILE.toc -a $MYLATEX = pdflatex ] ; then
	$MYDIR/toc2bkm.py -c section $MYFILE
    fi
    $MYLATEX $MYFILE || exit $?
}

build_dvi() {
    use_latex $1 latex
}

build_pdf() {
    use_latex $1 pdflatex
}

build_ps() {
    dvips -N0 -o $1.ps $1 || exit $?
}

cleanup() {
    rm -f $1.aux $1.log $1.out $1.toc $1.bkm $1.idx $1.ilg $1.ind
    rm -f mod$1.idx mod$1.ilg mod$1.ind
    if [ ! "$BUILD_DVI" ] ; then
	rm -f $1.dvi
    fi
}

# figure out what our targets are:
while [ "$1" ] ; do
    case "$1" in
	--pdf|--pd)
	    BUILD_PDF=true
	    USE_DEFAULT_FORMAT=false
	    shift 1
	    ;;
	--ps|--postscript|--postscrip|--postscri|--postscr|--postsc|--posts|--post|--pos|--po)
	    BUILD_PS=true
	    USE_DEFAULT_FORMAT=false
	    shift 1
	    ;;
	--dvi|--dv|--d)
	    BUILD_DVI=true
	    USE_DEFAULT_FORMAT=false
	    shift 1
	    ;;
	--html|--htm|--ht)
	    BUILD_HTML=true
	    USE_DEFAULT_FORMAT=false
	    shift 1
	    ;;
	-H|--help|--hel|--he)
	    usage 0
	    ;;
	-a|--address|--addres|--addre|-addr|--add|--ad)
	    ADDRESS="$2"
	    shift 2
	    ;;
	--a4)
	    TEXINPUTS=$TOPDIR/paper-a4:$TEXINPUTS
	    shift 1
	    ;;
	--letter|--lette|--lett|--let|--le)
	    shift 1
	    ;;
	--link|--lin|--li)
	    LINK="$2"
	    shift 2
	    ;;
	-s|--split|--spli|--spl|--sp|--s)
	    MAX_SPLIT_DEPTH="$2"
	    shift 2
	    ;;
	-l|--logging|--loggin|--loggi|--logg|--log|--lo)
	    LOGGING=true
	    shift 1
	    ;;
	-D|--debugging|--debuggin|--debuggi|--debugg|--debug|--debu|--deb|--de)
	    DEBUGGING=true
	    shift 1
	    ;;
	-k|--keep|--kee|--ke|--k)
	    DISCARD_TEMPS=''
	    shift 1
	    ;;
	-q|--quiet|--quie|--qui|--qu|--q)
	    QUIET=true
	    shift 1
	    ;;
	--)
	    shift 1
	    break
	    ;;
	-*)
	    usage 2
	    ;;
	*)
	    break;;
    esac
done

if [ $# = 0 ] ; then
    usage 2
fi

if [ $USE_DEFAULT_FORMAT = true ] ; then
    eval "BUILD_$DEFAULT_FORMAT=true"
fi

if [ "$DEBUGGING" ] ; then
    set -x
fi

if [ "$QUIET" ] ; then
    exec >/dev/null
fi

COMMONTEXINPUTS=$TOPDIR/texinputs:$TEXINPUTS

for FILE in $@ ; do
    FILEDIR=`dirname $FILE`
    FILE=`basename ${FILE%.tex}`
    #
    # Put the directory the .tex file is in is also the first directory in
    # TEXINPUTS, to allow files there to override files in the common area.
    #
    TEXINPUTS=$FILEDIR:$COMMONTEXINPUTS
    export TEXINPUTS
    #
    if [ "$BUILD_DVI" -o "$BUILD_PS" ] ; then
	build_dvi $FILE 2>&1 | tee -a $LOGFILE
    fi
    if [ "$BUILD_PDF" ] ; then
	build_pdf $FILE 2>&1 | tee -a $LOGFILE
    fi
    if [ "$BUILD_PS" ] ; then
	build_ps $FILE 2>&1 | tee -a $LOGFILE
    fi
    if [ "$BUILD_HTML" ] ; then
	if [ ! "$BUILD_DVI" -o ! "$BUILD_PDF" ] ; then
	    # need to get aux file
	    build_dvi $FILE 2>&1 | tee -a $LOGFILE
	fi
	build_html $FILE 2>&1 | tee -a $LOGFILE
    fi
    if [ "$DISCARD_TEMPS" ] ; then
	cleanup $FILE 2>&1 | tee -a $LOGFILE
    fi
    # keep the logfile around
    if [ "$LOGGING" ] ; then
	cp $LOGFILE $FILE.how
    fi
    rm -f $LOGFILE
done
