# SYNOPSIS
#
#   AX_CHECK_OPENSSL_STATIC()
#
# DESCRIPTION
#
#   Look for OpenSSL static libraries in user-selected spot (via --with-openssl-static).
#   Based on AX_CHECK_OPENSSL. Sets
#
#     OPENSSL_INCLUDES to the include directives required
#     OPENSSL_STATIC_ROOT to the path to libssl.a and libcrypto.a
#     WITH_OPENSSL_STATIC true/false - true if --with-openssl-static specified
#
#   This macro sets OPENSSL_INCLUDES such that source files should use the
#   openssl/ directory in include directives:
#
#     #include <openssl/hmac.h>

AC_DEFUN([AX_CHECK_OPENSSL_STATIC], [
    AC_ARG_WITH([openssl-static],
        [AS_HELP_STRING([--with-openssl-static=DIR],
            [root of the OpenSSL directory if you want to link OpenSSL statically.])],
        [
            case "$withval" in
            "" | y | ye | yes | n | no)
            AC_MSG_ERROR([Invalid --with-openssl-static value])
              ;;
            *) ssldir="$withval"
              ;;
            esac

            WITH_OPENSSL_STATIC=true
            AC_MSG_CHECKING([for openssl/ssl.h in $ssldir])
            if test -f "$ssldir/include/openssl/ssl.h"; then
                OPENSSL_INCLUDES="-I$ssldir/include"
                AC_MSG_RESULT([yes])
            else
                AC_MSG_RESULT([no])
                AC_MSG_ERROR(m4_normalize([Invalid --with-openssl-static value -
                    $ssldir/include/openssl/ssl.h does not exist]))
            fi

            AC_MSG_CHECKING([for libssl.a and libcrypto.a in $ssldir])
            if test -e "$ssldir/libssl.a" -a -e "$ssldir/libcrypto.a"; then
                OPENSSL_STATIC_ROOT="$ssldir"
                AC_MSG_RESULT([yes])
            else
                AC_MSG_RESULT([no])
                AC_MSG_CHECKING([for libssl.a and libcrypto.a in $ssldir/lib])
                if test -e "$ssldir/lib/libssl.a" -a -e "$ssldir/lib/libcrypto.a"; then
                    AC_MSG_RESULT([yes])
                    OPENSSL_STATIC_ROOT="$ssldir/lib"
                else
                    AC_MSG_RESULT([no])
                    AC_MSG_ERROR(m4_normalize([Invalid --with-openssl-static value -
                        libssl.a and/or libcrypto.a not found]))
                fi
            fi

            # try the preprocessor and linker with our new flags,
            # being careful not to pollute the global LIBS, LDFLAGS, and CPPFLAGS

            save_LIBS="$LIBS"
            save_LDFLAGS="$LDFLAGS"
            save_CPPFLAGS="$CPPFLAGS"

            echo "Trying link statically with;" \
                "OPENSSL_LIBS=$OPENSSL_STATIC_ROOT/libssl.a $OPENSSL_STATIC_ROOT/libcrypto.a; " \
                "OPENSSL_INCLUDES=$OPENSSL_INCLUDES" >&AS_MESSAGE_LOG_FD

            LIBS="$OPENSSL_STATIC_ROOT/libssl.a $OPENSSL_STATIC_ROOT/libcrypto.a $LIBS"
            CPPFLAGS="$OPENSSL_INCLUDES $CPPFLAGS"

            AC_MSG_CHECKING([if compiling and linking against OpenSSL works])
            AC_LINK_IFELSE(
                [AC_LANG_PROGRAM([#include <openssl/ssl.h>], [SSL_new(NULL)])],
                [
                    AC_MSG_RESULT([yes])
                ], [
                    AC_MSG_RESULT([no])
                    AC_MSG_ERROR([Can't link against OpenSSL in $OPENSSL_STATIC_ROOT])
                ]
            )

            AC_MSG_CHECKING([if OpenSSL is compiled with -fPIC])

            LDFLAGS="-shared -fPIC $LDFLAGS"
            AC_LINK_IFELSE(
                [AC_LANG_SOURCE([[#include <openssl/ssl.h>
                void test() {
                    SSL_new(NULL);
                }]])],
                [
                    AC_MSG_RESULT([yes])
                ], [
                    AC_MSG_RESULT([no])
                    AC_MSG_ERROR(m4_normalize([Can't link shared objects against OpenSSL in
                        $OPENSSL_STATIC_ROOT. Is OpenSSL compiled with -fPIC ?]))
                ])

            CPPFLAGS="$save_CPPFLAGS"
            LDFLAGS="$save_LDFLAGS"
            LIBS="$save_LIBS"

            AC_SUBST([OPENSSL_INCLUDES])
            AC_SUBST([OPENSSL_STATIC_ROOT])
        ],
        [
            WITH_OPENSSL_STATIC=false
        ])
])
