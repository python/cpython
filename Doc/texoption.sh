#! /bin/sh
#
#  Script to convert LaTeX2e documents to & from having a \documentclass{}
#  option.

VERBOSE=false
SETOPTION=true
FILES=`echo ???.tex`

usage() {
    exec >&2
    echo "usage:	$0 [-d] [-v] option [files...]"
    echo
    echo "\t-d  disable option, if present"
    echo "\t-v  tell which files are being edited, and how"
    echo
    echo "\tBy default, files... will be '???.tex'."
    echo "\tThis will match each of the Python manuals."
    echo
    exit 2
}

editing() {
    # tell the user what we're doing
    if $VERBOSE ; then
	echo $1 $FILE...
    fi
}

addoption() {
    # add an option not already present
    editing "adding to"
    (sed 's/^\(\\documentclass[[].*\)]/\1,'$OPTION']/
s/^\\documentclass{/\\documentclass['$OPTION']{/' $FILE >temp-$$ \
     && mv temp-$$ $FILE) || exit $?
}

remoption() {
    # remove an option currently on
    editing "removing from"
    (sed 's/^\(\\documentclass[[].*\),'$OPTION'\([],]\)/\1\2/
s/^\\documentclass[[]'$OPTION']/\\documentclass{/
s/^\\documentclass[[]'$OPTION',/\\documentclass[/' $FILE >temp-$$ \
     && mv temp-$$ $FILE) || exit $?
}

chkoption() {
    # return true iff the option is already on
    egrep '^\\documentclass[[]([A-Za-z0-9]*,)*'$OPTION'[],]' $FILE >/dev/null
    return $?
}

# parse the command line...
while [ "$#" -gt 0 ] ; do
    case "$1" in
	-d)
	    SETOPTION=false
	    shift
	    ;;
	-v)
	    VERBOSE=true
	    shift
	    ;;
	-*)
	    usage
	    ;;
	*)
	    break;
	    ;;
    esac
done
if [ -z "$1" ] ; then
    usage
fi

# setup variables
OPTION="$1"
shift
FILES=${1:+$*}
if [ "$FILES" = '' ] ; then
    FILES=`echo ???.tex`
fi

# check each file and do the work as required
for FILE in $FILES ; do
    if chkoption ; then
	if $SETOPTION ; then
	    :
	else
	    remoption
	fi
    elif $SETOPTION ; then
	addoption
    fi
done
