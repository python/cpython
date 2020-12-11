
#------------------------------------------------------------------------
# NS_PATH_AOLSERVER
#
#   Allows the building with support for NaviServer/AOLserver
#
# Arguments:
#   none
#
# Results:
#
#   Adds the following arguments to configure:
#       --with-naviserver=...
#
#   Defines the following vars:
#       NS_DIR Full path to the directory containing NaviServer/AOLserver distro
#       NS_INCLUDES
#       NS_LIBS
#
#   Sets the following vars:
#       NS_AOLSERVER
#
#   Updates following vars:
#------------------------------------------------------------------------

AC_DEFUN(NS_PATH_AOLSERVER, [
    AC_MSG_CHECKING([for NaviServer/AOLserver configuration])
    AC_ARG_WITH(naviserver,
    [  --with-naviserver       directory with NaviServer/AOLserver distribution],\
    with_naviserver=${withval})

    AC_CACHE_VAL(ac_cv_c_naviserver,[
    if test x"${with_naviserver}" != x ; then
        if test -f "${with_naviserver}/include/ns.h" ; then
            ac_cv_c_naviserver=`(cd ${with_naviserver}; pwd)`
        else
            AC_MSG_ERROR([${with_naviserver} directory doesn't contain ns.h])
        fi
    fi
    ])
    if test x"${ac_cv_c_naviserver}" = x ; then
        AC_MSG_RESULT([none found])
    else
        NS_DIR=${ac_cv_c_naviserver}
        AC_MSG_RESULT([found NaviServer/AOLserver in $NS_DIR])
        NS_INCLUDES="-I\"${NS_DIR}/include\""
        if test "`uname -s`" = Darwin ; then
            aollibs=`ls ${NS_DIR}/lib/libns* 2>/dev/null`
            if test x"$aollibs" != x ; then
                NS_LIBS="-L\"${NS_DIR}/lib\" -lnsd -lnsthread"
            fi
        fi
        AC_DEFINE(NS_AOLSERVER)
    fi
])

# EOF
