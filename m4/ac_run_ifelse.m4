m4_rename([AC_RUN_IFELSE], [_AX_RUN_IFELSE])

# AC_RUN_IFELSE(PROGRAM,
#               [ACTION-IF-TRUE], [ACTION-IF-FALSE],
#               [ACTION-IF-CROSS-COMPILING = RUNTIME-ERROR])
# ----------------------------------------------------------
# Compile, link, and run.
# If cross-compiling and ACTION-IF-TRUE begins by setting the value of a
# variable, create a directory after the name of this variable in
# build/run_ifelse_cvs with the following files:
#   'TRUE'            contains ACTION-IF-TRUE
#   'FALSE'           contains ACTION-IF-FALSE
#   'CROSS-COMPILING' contains ACTION-IF-CROSS-COMPILING
#   'a.out'           executable, product of AC_LINK_IFELSE
#   'conftest.c'      source code if the link failed

dnl Quadrigraphs "@<:@" and "@:>@" produce "[" and "]" in the output.
dnl Unfortunately, we must resort to using those quadrigraphs as AC_RUN_IFELSE
dnl macros are not all quoted in configure.ac (as they should).
AC_DEFUN([AC_RUN_IFELSE],
         [AS_IF([test "$cross_compiling" = yes -a -z "$CONFIG_SITE"],
                [AS_TMPDIR([ac_r])
                 _AX_VAR_CAT([$2], [$tmp/ACTION-IF-TRUE])
                 ax_run_ifelse_var=`cat $tmp/ACTION-IF-TRUE | LC_ALL=C sed -n \
                   -e '/^@<:@ \t\n@:>@*$/d' \
                   -e 's/^@<:@ \t\n@:>@*\(@<:@_a-zA-Z@:>@@<:@_0-9a-zA-Z@:>@*\)=.*/\1/p' \
                   -e 'q'`
                 rm -r $tmp
                 AS_IF([test -n "$ax_run_ifelse_var"],
                       [ax_run_ifelse_var=build/run_ifelse_cvs/$ax_run_ifelse_var
                        AS_MKDIR_P([$ax_run_ifelse_var])
                        _AX_VAR_CAT([$2], [$ax_run_ifelse_var/TRUE])
                        _AX_VAR_CAT([$3], [$ax_run_ifelse_var/FALSE])
                        _AX_VAR_CAT([$4], [$ax_run_ifelse_var/CROSS-COMPILING])
                        AC_LINK_IFELSE([$1],
                                       [cat conftest$EXEEXT > $ax_run_ifelse_var/a.out
                                        chmod a+x $ax_run_ifelse_var/a.out],
                                       [cat conftest.c > $ax_run_ifelse_var/conftest.c])
                 ])
          ])
          _AX_RUN_IFELSE($@)
])# AC_RUN_IFELSE


# _AX_VAR_CAT(CONTENT, FILE-NAME)
# --------------------------------------
# Copy CONTENT to FILE-NAME.

AC_DEFUN([_AX_VAR_CAT],
         [cat <<AXEOF > $2
$1
AXEOF
])# _AX_VAR_CAT
