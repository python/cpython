#!/bin/sh

# A script that runs each unit test independently, with output
# directed to a file in OUT/$T.out.  If command line arguments are
# given, they are tests to run; otherwise every file named
# Lib/test/test_* is run (via regrtest).  A summary of failing,
# passing and skipped tests is written to stdout and to the files
# GOOD, BAD and SKIPPED.

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

# The -u flag (edit this file to change).
UFLAG=""

# Compute the list of tests to run.
case $# in
0) 
    TESTS=`(cd Lib/test; ls test_*.py | sed 's/\.py//')`
    ;;
*)
    TESTS="$@"
    ;;
esac

# Run the tests.
for T in $TESTS
do
    echo -n $T
    if $PYTHON Lib/test/regrtest.py $UFLAG $T >OUT/$T.out 2>&1
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
	##echo "--------- Re-running test in verbose mode ---------" >>OUT/$T.out
	##$PYTHON Lib/test/regrtest.py -v $UFLAG $T >>OUT/$T.out 2>&1
    fi
done
