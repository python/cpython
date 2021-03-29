#! /bin/sh

TOP_DIR=$(dirname $0)
LAST_DIR=$PWD

if test ! -f $TOP_DIR/configure.ac ; then
   echo "You must execute this script from the top level directory."
   exit 1
fi

AUTOCONF=${AUTOCONF:-autoconf}
ACLOCAL=${ACLOCAL:-aclocal}
AUTOMAKE=${AUTOMAKE:-automake}
AUTOHEADER=${AUTOHEADER:-autoheader}
LIBTOOLIZE=${LIBTOOLIZE:-libtoolize}
#SHTOOLIZE=${SHTOOLIZE:-shtoolize}

dump_help_screen ()
{
   echo "Usage: $0 [options]"
   echo 
   echo "options:"
   echo "  -n           skip CVS changelog creation"
   echo "  -h,--help    show this help screen"
   echo
   exit 0
}

parse_options ()
{
   while test "$1" != "" ; do
      case $1 in
         -h|--help)
            dump_help_screen
            ;;
         -n)
            SKIP_CVS_CHANGELOG=yes
            ;;
         *)
            echo Invalid argument - $1
            dump_help_screen
            ;;
      esac
      shift
   done
}

run_or_die ()
{
   COMMAND=$1
   
   # check for empty commands
   if test -z "$COMMAND" ; then
      echo "*warning* no command specified"
      return 1
   fi
   
   shift;

   OPTIONS="$@"
   
   # print a message
   echo -n "*info* running $COMMAND"
   if test -n "$OPTIONS" ; then
      echo " ($OPTIONS)"
   else
      echo
   fi

   # run or die
   $COMMAND $OPTIONS ; RESULT=$?
   if test $RESULT -ne 0 ; then
      echo "*error* $COMMAND failed. (exit code = $RESULT)"
      exit 1
   fi
   
   return 0
}

parse_options "$@"

echo "Building librb autotools files."

cd "$TOP_DIR"/librb
sh autogen.sh

echo "Building main autotools files."

cd "$LAST_DIR"

run_or_die $ACLOCAL -I m4
run_or_die $LIBTOOLIZE --force --copy
run_or_die $AUTOHEADER
run_or_die $AUTOCONF
run_or_die $AUTOMAKE --add-missing --copy
#run_or_die $SHTOOLIZE all
