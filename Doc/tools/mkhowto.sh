#! /usr/bin/env bash

# This script may be invoked by naming it directly or via a shell alias,
# but NOT through a symbolic link.  Perhaps a future version will allow
# the use of a symbolic link.
#
# Using a symbolic link will cause the script to not be able to locate
# its support files.

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

ICONSERVER=''

L2H_INIT_FILE=$TOPDIR/perl/l2hinit.perl
L2H_AUX_INIT_FILE=/usr/tmp/mkhowto-$LOGNAME-$$.perl

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
    --text		ASCII text

    More than one output format may be specified.

HTML options:
    --address, -a	Specify an address for page footers.
    --link		Specify the number of levels to include on each page.
    --split, -s		Specify a section level for page splitting.
    --iconserver, -i	Specify location of icons (default: ../).

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
    BUILDDIR=${2:-$1}
    latex2html \
     -init_file $L2H_INIT_FILE \
     -init_file $L2H_AUX_INIT_FILE \
     -dir $BUILDDIR $TEXFILE || exit $?
    if [ "$MAX_SPLIT_DEPTH" -ne 1 ] ; then
	(cd $BUILDDIR; $MYDIR/node2label.pl *.html) || exit $?
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
    if [ -f $MYFILE.syn ] ; then
	# This hack is due to a bug with the module synopsis support that
	# causes the last module synopsis to be written out twice in
	# howto documents (not present for manuals).  Repeated below.
	uniq $MYFILE.syn >TEMP.syn && mv TEMP.syn $MYFILE.syn || exit $?
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
    if [ -f $MYFILE.syn ] ; then
	uniq $MYFILE.syn >TEMP.syn && mv TEMP.syn $MYFILE.syn || exit $?
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

build_text() {
    lynx -nolist -dump $2/index.html >$1.txt
}

l2hoption() {
    if [ "$2" ] ; then
	echo "\$$1 = \"$2\";" >>$L2H_AUX_INIT_FILE
    fi
}

cleanup() {
    rm -f $1.aux $1.log $1.out $1.toc $1.bkm $1.idx $1.ilg $1.ind $1.syn
    rm -f mod$1.idx mod$1.ilg mod$1.ind
    if [ ! "$BUILD_DVI" ] ; then
	rm -f $1.dvi
    fi
    rm -rf $1.temp-html
    rm -f $1/IMG* $1/*.pl $1/WARNINGS $1/index.dat $1/modindex.dat
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
	--text|--tex|--te|--t)
	    BUILD_TEXT=true
	    USE_DEFAULT_FORMAT=false
	    shift 1
	    ;;
	-H|--help|--hel|--he)
	    usage 0
	    ;;
	-i|--iconserver|--iconserve|--iconserv|--iconser|--iconse|--icons|--icon|--ico|--ic|--i)
	    ICONSERVER="$2"
	    shift 2
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
    # check for a single .tex file in .
    COUNT=`ls -1 *.tex | wc -l | sed 's/[ 	]//g'`
    if [ "$COUNT" -eq 1 ] ; then
	set -- `ls -1 *.tex`
    else
	usage 2
    fi
fi

if [ $USE_DEFAULT_FORMAT = true ] ; then
    eval "BUILD_$DEFAULT_FORMAT=true"
fi

if [ "$QUIET" ] ; then
    exec >/dev/null
fi

if [ "$DEBUGGING" ] ; then
    set -x
fi

echo '# auxillary init file for latex2html' >$L2H_AUX_INIT_FILE
echo '# generated by mkhowto.sh -- do no edit' >>$L2H_AUX_INIT_FILE
l2hoption ICONSERVER "$ICONSERVER"
l2hoption ADDRESS "$ADDRESS"
l2hoption MAX_LINK_DEPTH "$MAX_LINK_DEPTH"
l2hoption MAX_SPLIT_DEPTH "$MAX_SPLIT_DEPTH"
echo '1;' >>$L2H_AUX_INIT_FILE

for FILE in $@ ; do
    FILEDIR=`dirname $FILE`
    FILE=`basename ${FILE%.tex}`
    #
    # Put the directory the .tex file is in is also the first directory in
    # TEXINPUTS, to allow files there to override files in the common area.
    #
    TEXINPUTS=$FILEDIR:$TOPDIR/texinputs:$TEXINPUTS
    export TEXINPUTS
    #
    if [ "$BUILD_DVI" -o "$BUILD_PS" ] ; then
	build_dvi $FILE 2>&1 | tee -a $LOGFILE
	HAVE_TEMPS=true
    fi
    if [ "$BUILD_PDF" ] ; then
	build_pdf $FILE 2>&1 | tee -a $LOGFILE
	HAVE_TEMPS=true
    fi
    if [ "$BUILD_PS" ] ; then
	build_ps $FILE 2>&1 | tee -a $LOGFILE
    fi
    if [ "$BUILD_HTML" ] ; then
	if [ ! "$HAVE_TEMPS" ] ; then
	    # need to get aux file
	    build_dvi $FILE 2>&1 | tee -a $LOGFILE
	    HAVE_TEMPS=true
	fi
	build_html $FILE $FILE 2>&1 | tee -a $LOGFILE
    fi
    if [ "$BUILD_TEXT" ] ; then
	if [ ! "$HAVE_TEMPS" ] ; then
	    # need to get aux file
	    build_dvi $FILE 2>&1 | tee -a $LOGFILE
	    HAVE_TEMPS=true
	fi
	# this is why building text really has to be last:
	if [ "$MAX_SPLIT_DEPTH" -ne 1 ] ; then
	    echo '# re-hack this file for --text:' >>$L2H_AUX_INIT_FILE
	    l2hoption MAX_SPLIT_DEPTH 1
	    echo '1;' >>$L2H_AUX_INIT_FILE
	    TEMPDIR=$FILE.temp-html
	    build_html $FILE $TEMPDIR 2>&1 | tee -a $LOGFILE
	else
	    TEMPDIR=$FILE
	fi
	build_text $FILE $TEMPDIR 2>&1 | tee -a $LOGFILE
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

rm -f $L2H_AUX_INIT_FILE
