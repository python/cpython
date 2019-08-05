# ===============================================================================
#  https://www.gnu.org/software/autoconf-archive/ax_c_float_words_bigendian.html
# ===============================================================================
#
# SYNOPSIS
#
#   AX_C_FLOAT_WORDS_BIGENDIAN([ACTION-IF-TRUE], [ACTION-IF-FALSE], [ACTION-IF-UNKNOWN])
#
# DESCRIPTION
#
#   Checks the ordering of words within a multi-word float. This check is
#   necessary because on some systems (e.g. certain ARM systems), the float
#   word ordering can be different from the byte ordering. In a multi-word
#   float context, "big-endian" implies that the word containing the sign
#   bit is found in the memory location with the lowest address. This
#   implementation was inspired by the AC_C_BIGENDIAN macro in autoconf.
#
#   The endianness is detected by first compiling C code that contains a
#   special double float value, then grepping the resulting object file for
#   certain strings of ASCII values. The double is specially crafted to have
#   a binary representation that corresponds with a simple string. In this
#   implementation, the string "noonsees" was selected because the
#   individual word values ("noon" and "sees") are palindromes, thus making
#   this test byte-order agnostic. If grep finds the string "noonsees" in
#   the object file, the target platform stores float words in big-endian
#   order. If grep finds "seesnoon", float words are in little-endian order.
#   If neither value is found, the user is instructed to specify the
#   ordering.
#
# LICENSE
#
#   Copyright (c) 2008 Daniel Amelang <dan@amelang.net>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 11

AC_DEFUN([AX_C_FLOAT_WORDS_BIGENDIAN],
  [AC_CACHE_CHECK(whether float word ordering is bigendian,
                  ax_cv_c_float_words_bigendian, [

ax_cv_c_float_words_bigendian=unknown
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[

double d = 90904234967036810337470478905505011476211692735615632014797120844053488865816695273723469097858056257517020191247487429516932130503560650002327564517570778480236724525140520121371739201496540132640109977779420565776568942592.0;

]])], [

if grep noonsees conftest.$ac_objext >/dev/null ; then
  ax_cv_c_float_words_bigendian=yes
fi
if grep seesnoon conftest.$ac_objext >/dev/null ; then
  if test "$ax_cv_c_float_words_bigendian" = unknown; then
    ax_cv_c_float_words_bigendian=no
  else
    ax_cv_c_float_words_bigendian=unknown
  fi
fi

])])

case $ax_cv_c_float_words_bigendian in
  yes)
    m4_default([$1],
      [AC_DEFINE([FLOAT_WORDS_BIGENDIAN], 1,
                 [Define to 1 if your system stores words within floats
                  with the most significant word first])]) ;;
  no)
    $2 ;;
  *)
    m4_default([$3],
      [AC_MSG_ERROR([

Unknown float word ordering. You need to manually preset
ax_cv_c_float_words_bigendian=no (or yes) according to your system.

    ])]) ;;
esac

])# AX_C_FLOAT_WORDS_BIGENDIAN
