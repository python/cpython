#!/bin/sh

HELP="Usage: ./runtests.py [-h] [-x] [flags] [tests]

Runs each unit test independently, with output directed to a file in
OUT/<test>.out.  If no tests are given, all tests are run; otherwise,
only the specified tests are run, unless -x is also given, in which
case all tests *except* those given are run.

Standard output shows the name of the tests run, with 'BAD' or
'SKIPPED' added if the test didn't produce a positive result.  Also,
three files are created, named 'BAD', 'GOOD' and 'SKIPPED', to which
are written the names of the tests categorized by result.

Flags (arguments starting with '-') are passed transparently to
regrtest.py, except for -x, which is processed here."

# Reset PYTHONPATH to avoid alien influences on the tests.
unset PYTHONPATH

# Choose the Python binary.
case `uname` in
Darwin) PYTHON=./python.exe;;
CYGWIN*) PYTHON=./python.exe;;
*)      PYTHON=./python;;
esac

# Create the output directory if necessary.
mkdir -p OUT

# Empty the summary files.
>GOOD
>BAD
>SKIPPED

# Process flags (transparently pass these on to regrtest.py)
FLAGS=""
EXCEPT=""
while :
do
    case $1 in
    -h|--h|-help|--help) echo "$HELP"; exit;;
    --) FLAGS="$FLAGS $1"; shift; break;;
    -x) EXCEPT="$1"; shift;;
    -*) FLAGS="$FLAGS $1"; shift;;
    *)  break;;
    esac
done

# Compute the list of tests to run.
case "$#$EXCEPT" in
0) 
    TESTS=`(cd Lib/test; ls test_*.py | sed 's/\.py//')`
    ;;
*-x)
    PAT="^(`echo $@ | sed 's/\.py//' | sed 's/ /|/'`)$"
    TESTS=`(cd Lib/test; ls test_*.py | sed 's/\.py//' | egrep -v "$PAT")`
    ;;
*)
    TESTS="$@"
    ;;
esac

# Run the tests.
for T in $TESTS
do
    echo -n $T
    if   case $T in
         *curses*) echo; $PYTHON Lib/test/regrtest.py $FLAGS $T 2>OUT/$T.out;;
         *) $PYTHON Lib/test/regrtest.py $FLAGS $T >OUT/$T.out 2>&1;;
         esac
    then
        if grep -q "1 test skipped:" OUT/$T.out
        then
            echo " SKIPPED"
            echo $T >>SKIPPED
        else
            echo
            echo $T >>GOOD
        fi
    else
        echo " BAD"
        echo $T >>BAD
    fi
done

# Summarize results
wc -l BAD GOOD SKIPPED
