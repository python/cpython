#! /bin/sh

#  Script to push docs from my development area to SourceForge, where the
#  update-docs.sh script unpacks them into their final destination.

TARGETHOST=www.python.org
TARGETDIR=/usr/home/fdrake/tmp

TARGET="$TARGETHOST:$TARGETDIR"

ADDRESSES='python-dev@python.org doc-sig@python.org python-list@python.org'

TOOLDIR="`dirname $0`"
VERSION=`$TOOLDIR/getversioninfo`

# Set $EXTRA to something non-empty if this is a non-trunk version:
EXTRA=`echo "$VERSION" | sed 's/^[0-9][0-9]*\.[0-9][0-9]*//'`

if echo "$EXTRA" | grep -q '[.]' ; then
    DOCLABEL="maintenance"
    DOCTYPE="maint"
else
    DOCLABEL="development"
    DOCTYPE="devel"
fi

EXPLANATION=''
ANNOUNCE=true

while [ "$#" -gt 0 ] ; do
  case "$1" in
      -m)
          EXPLANATION="$2"
          shift 2
          ;;
      -q)
          ANNOUNCE=false
          shift 1
          ;;
      -t)
          DOCTYPE="$2"
          shift 2
          ;;
      -F)
          EXPLANATION="`cat $2`"
          shift 2
          ;;
      -*)
          echo "Unknown option: $1" >&2
          exit 2
          ;;
      *)
          break
          ;;
  esac
done
if [ "$1" ] ; then
    if [ "$EXPLANATION" ] ; then
        echo "Explanation may only be given once!" >&2
        exit 2
    fi
    EXPLANATION="$1"
    shift
fi

if [ "$DOCTYPE" = 'maint' ] ; then
    # 'maint' is a symlink
    DOCTYPE='maint23'
fi

START="`pwd`"
MYDIR="`dirname $0`"
cd "$MYDIR"
MYDIR="`pwd`"

cd ..

# now in .../Doc/
make --no-print-directory bziphtml || exit $?
PACKAGE="html-$VERSION.tar.bz2"
scp "$PACKAGE" tools/update-docs.sh $TARGET/ || exit $?
ssh "$TARGETHOST" tmp/update-docs.sh $DOCTYPE $PACKAGE '&&' rm tmp/update-docs.sh || exit $?

if $ANNOUNCE ; then
    sendmail $ADDRESSES <<EOF
To: $ADDRESSES
From: "Fred L. Drake" <fdrake@acm.org>
Subject: [$DOCLABEL doc updates]
X-No-Archive: yes

The $DOCLABEL version of the documentation has been updated:

    http://$TARGETHOST/dev/doc/$DOCTYPE/

$EXPLANATION
EOF
    exit $?
fi
