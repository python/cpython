/* Features part to handle 64-bit time_t support.
   Copyright (C) 2021-2022 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

/* We need to know the word size in order to check the time size.  */
#include <bits/wordsize.h>
#include <bits/timesize.h>

#if defined _TIME_BITS
# if _TIME_BITS == 64
#  if ! defined (_FILE_OFFSET_BITS) || _FILE_OFFSET_BITS != 64
#   error "_TIME_BITS=64 is allowed only with _FILE_OFFSET_BITS=64"
#  elif __TIMESIZE == 32
#   define __USE_TIME_BITS64	1
#  endif
# elif _TIME_BITS == 32
#  if __TIMESIZE > 32
#   error "_TIME_BITS=32 is not compatible with __TIMESIZE > 32"
#  endif
# else
#  error Invalid _TIME_BITS value (can only be 32 or 64-bit)
# endif
#endif
