#!/bin/sh

## Script to build and test the latest python from svn.  It basically
## does this:
##   svn up ; ./configure ; make ; make test ; make install ; cd Doc ; make
##
## Logs are kept and rsync'ed to the host.  If there are test failure(s),
## information about the failure(s) is mailed.
##
## This script is run on the PSF's machine as user neal via crontab.
##
## Yes, this script would probably be easier in python, but then
## there's a bootstrap problem.  What if Python doesn't build?
##
## This script should be fairly clean Bourne shell, ie not too many
## bash-isms.  We should try to keep it portable to other Unixes.
## Even though it will probably only run on Linux.  I'm sure there are
## several GNU-isms currently (date +%s and readlink).
##
## Perhaps this script should be broken up into 2 (or more) components.
## Building doc is orthogonal to the rest of the python build/test.
##

## FIXME: we should detect test hangs (eg, if they take more than 45 minutes)

## FIXME: we should run valgrind
## FIXME: we should run code coverage

## Utilities invoked in this script include:
##    basename, date, dirname, expr, grep, readlink, uname
##    cksum, make, mutt, rsync, svn

## remember where did we started from
DIR=`dirname $0`
if [ "$DIR" = "" ]; then
   DIR="."
fi

## make directory absolute
DIR=`readlink -f $DIR`
FULLPATHNAME="$DIR/`basename $0`"
## we want Misc/..
DIR=`dirname $DIR`

## Configurable options

FAILURE_SUBJECT="Python Regression Test Failures"
#FAILURE_MAILTO="YOUR_ACCOUNT@gmail.com"
FAILURE_MAILTO="python-3000-checkins@python.org"
#FAILURE_CC="optional--uncomment and set to desired address"

REMOTE_SYSTEM="neal@dinsdale.python.org"
REMOTE_DIR="/data/ftp.python.org/pub/docs.python.org/dev/3.0"
RESULT_FILE="$DIR/build/index.html"
INSTALL_DIR="/tmp/python-test-3.0/local"
RSYNC_OPTS="-aC -e ssh"

# Always run the installed version of Python.
PYTHON=$INSTALL_DIR/bin/python

# Python options and regression test program that should always be run.
REGRTEST_ARGS="-E -tt $INSTALL_DIR/lib/python3.0/test/regrtest.py"

REFLOG="build/reflog.txt.out"
# These tests are not stable and falsely report leaks sometimes.
# The entire leak report will be mailed if any test not in this list leaks.
# Note: test_XXX (none currently) really leak, but are disabled
# so we don't send spam.  Any test which really leaks should only 
# be listed here if there are also test cases under Lib/test/leakers.
LEAKY_TESTS="test_(asynchat|cmd_line|popen2|socket|smtplib|sys|threadsignals|urllib2_localnet)"

# These tests always fail, so skip them so we don't get false positives.
_ALWAYS_SKIP=""
ALWAYS_SKIP="-x $_ALWAYS_SKIP"

# Skip these tests altogether when looking for leaks.  These tests
# do not need to be stored above in LEAKY_TESTS too.
# test_logging causes hangs, skip it.
LEAKY_SKIPS="-x test_logging $_ALWAYS_SKIP"

# Change this flag to "yes" for old releases to only update/build the docs.
BUILD_DISABLED="no"

## utility functions
current_time() {
    date +%s
}

update_status() {
    now=`current_time`
    time=`expr $now - $3`
    echo "<li><a href=\"$2\">$1</a> <font size=\"-1\">($time seconds)</font></li>" >> $RESULT_FILE
}

place_summary_first() {
    testf=$1
    sed -n '/^[0-9][0-9]* tests OK\./,$p' < $testf \
        | egrep -v '\[[0-9]+ refs\]' > $testf.tmp
    echo "" >> $testf.tmp
    cat $testf >> $testf.tmp
    mv $testf.tmp $testf
}

count_failures () {
    testf=$1
    n=`grep -ic " failed:" $testf`
    if [ $n -eq 1 ] ; then
        n=`grep " failed:" $testf | sed -e 's/ .*//'`
    fi
    echo $n
}

mail_on_failure() {
    if [ "$NUM_FAILURES" != "0" ]; then
        dest=$FAILURE_MAILTO
        # FAILURE_CC is optional.
        if [ "$FAILURE_CC" != "" ]; then
            dest="$dest -c $FAILURE_CC"
        fi
        if [ "x$3" != "x" ] ; then
            (echo "More important issues:"
             echo "----------------------"
             egrep -v "$3" < $2
             echo ""
             echo "Less important issues:"
             echo "----------------------"
             egrep "$3" < $2)
        else
            cat $2
        fi | mutt -s "$FAILURE_SUBJECT $1 ($NUM_FAILURES)" $dest
    fi
}

## setup
cd $DIR
mkdir -p build
rm -f $RESULT_FILE build/*.out
rm -rf $INSTALL_DIR

## create results file
TITLE="Automated Python Build Results"
echo "<html>" >> $RESULT_FILE
echo "  <head>" >> $RESULT_FILE
echo "    <title>$TITLE</title>" >> $RESULT_FILE
echo "    <meta http-equiv=\"refresh\" content=\"43200\">" >> $RESULT_FILE
echo "  </head>" >> $RESULT_FILE
echo "<body>" >> $RESULT_FILE
echo "<h2>Automated Python Build Results</h2>" >> $RESULT_FILE
echo "<table>" >> $RESULT_FILE
echo "  <tr>" >> $RESULT_FILE
echo "    <td>Built on:</td><td>`date`</td>" >> $RESULT_FILE
echo "  </tr><tr>" >> $RESULT_FILE
echo "    <td>Hostname:</td><td>`uname -n`</td>" >> $RESULT_FILE
echo "  </tr><tr>" >> $RESULT_FILE
echo "    <td>Platform:</td><td>`uname -srmpo`</td>" >> $RESULT_FILE
echo "  </tr>" >> $RESULT_FILE
echo "</table>" >> $RESULT_FILE
echo "<ul>" >> $RESULT_FILE

## update, build, and test
ORIG_CHECKSUM=`cksum $FULLPATHNAME`
F=svn-update.out
start=`current_time`
svn update >& build/$F
err=$?
update_status "Updating" "$F" $start
if [ $err = 0 -a "$BUILD_DISABLED" != "yes" ]; then
    ## FIXME: we should check if this file has changed.
    ##  If it has changed, we should re-run the script to pick up changes.
    if [ "$ORIG_CHECKSUM" != "$ORIG_CHECKSUM" ]; then
        exec $FULLPATHNAME $@
    fi

    F=svn-stat.out
    start=`current_time`
    svn stat >& build/$F
    ## ignore some of the diffs
    NUM_DIFFS=`egrep -vc '^.      (@test|db_home|Lib/test/(regrtest\.py|db_home))$' build/$F`
    update_status "svn stat ($NUM_DIFFS possibly important diffs)" "$F" $start

    F=configure.out
    start=`current_time`
    ./configure --prefix=$INSTALL_DIR --with-pydebug >& build/$F
    err=$?
    update_status "Configuring" "$F" $start
    if [ $err = 0 ]; then
        F=make.out
        start=`current_time`
        make >& build/$F
        err=$?
        warnings=`grep warning build/$F | egrep -vc "te?mpnam(_r|)' is dangerous,"`
        update_status "Building ($warnings warnings)" "$F" $start
        if [ $err = 0 ]; then
            ## make install
            F=make-install.out
            start=`current_time`
            make install >& build/$F
            update_status "Installing" "$F" $start

            if [ ! -x $PYTHON ]; then
                ln -s ${PYTHON}3.* $PYTHON
            fi

            ## make and run basic tests
            F=make-test.out
            start=`current_time`
            $PYTHON $REGRTEST_ARGS -u urlfetch >& build/$F
            NUM_FAILURES=`count_failures build/$F`
            place_summary_first build/$F
            update_status "Testing basics ($NUM_FAILURES failures)" "$F" $start
            mail_on_failure "basics" build/$F

            F=make-test-opt.out
            start=`current_time`
            $PYTHON -O $REGRTEST_ARGS -u urlfetch >& build/$F
            NUM_FAILURES=`count_failures build/$F`
            place_summary_first build/$F
            update_status "Testing opt ($NUM_FAILURES failures)" "$F" $start
            mail_on_failure "opt" build/$F

            ## run the tests looking for leaks
            F=make-test-refleak.out
            start=`current_time`
            ## ensure that the reflog exists so the grep doesn't fail
            touch $REFLOG
            $PYTHON $REGRTEST_ARGS -R 4:3:$REFLOG -u network,urlfetch $LEAKY_SKIPS >& build/$F
            LEAK_PAT="($LEAKY_TESTS|sum=0)"
            NUM_FAILURES=`egrep -vc "$LEAK_PAT" $REFLOG`
            place_summary_first build/$F
            update_status "Testing refleaks ($NUM_FAILURES failures)" "$F" $start
            mail_on_failure "refleak" $REFLOG "$LEAK_PAT"

            ## now try to run all the tests
            F=make-testall.out
            start=`current_time`
            ## skip curses when running from cron since there's no terminal
            ## skip sound since it's not setup on the PSF box (/dev/dsp)
            $PYTHON $REGRTEST_ARGS -uall -x test_curses test_linuxaudiodev test_ossaudiodev $_ALWAYS_SKIP >& build/$F
            NUM_FAILURES=`count_failures build/$F`
            place_summary_first build/$F
            update_status "Testing all except curses and sound ($NUM_FAILURES failures)" "$F" $start
            mail_on_failure "all" build/$F
        fi
    fi
fi


## make doc
cd $DIR/Doc
F="make-doc.out"
start=`current_time`
# XXX(nnorwitz): For now, keep the code that checks for a conflicted file until
# after the first release of 2.6a1 or 3.0a1.  At that point, it will be clear
# if there will be a similar problem with the new doc system.

# Doc/commontex/boilerplate.tex is expected to always have an outstanding
# modification for the date.  When a release is cut, a conflict occurs.
# This allows us to detect this problem and not try to build the docs
# which will definitely fail with a conflict. 
#CONFLICTED_FILE=commontex/boilerplate.tex
#conflict_count=`grep -c "<<<" $CONFLICTED_FILE`
conflict_count=0
if [ $conflict_count != 0 ]; then
    echo "Conflict detected in $CONFLICTED_FILE.  Doc build skipped." > ../build/$F
    err=1
else
    make update html >& ../build/$F
    err=$?
fi
update_status "Making doc" "$F" $start
if [ $err != 0 ]; then
    NUM_FAILURES=1
    mail_on_failure "doc" ../build/$F
fi

echo "</ul>" >> $RESULT_FILE
echo "</body>" >> $RESULT_FILE
echo "</html>" >> $RESULT_FILE

## copy results
rsync $RSYNC_OPTS build/html/* $REMOTE_SYSTEM:$REMOTE_DIR
cd ../build
rsync $RSYNC_OPTS index.html *.out $REMOTE_SYSTEM:$REMOTE_DIR/results/
