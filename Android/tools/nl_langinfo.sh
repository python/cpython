# Test that langinfo.h is broken as expected.

cat > nl_langinfo.c <<EOF
/* Uncomment the following two lines and set the declaration of nl_langinfo()
 * to test the assertion error.
#undef __ANDROID_API__
#define __ANDROID_API__ __ANDROID_API_FUTURE__
*/

#include <langinfo.h>

void foo()
{
    nl_langinfo(1);
}
EOF

# XXX This is broken on systems where the Python source dir is read-only.
. $PY_SRCDIR/Android/build-config
if ! $CC $CFLAGS -Werror=implicit-function-declaration -c nl_langinfo.c; then
    rc=$?
    echo "langinfo.h is broken as expected." >&2
else
    rc=$?
    echo "Assertion error: langinfo.h is not broken in this new version of the NDK."
    echo "Please update Include/pyport.h and revert the changes made in issue 28596."
fi
rm -f nl_langinfo.c nl_langinfo.o
exit $rc
