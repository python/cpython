/*
                            __  __            _
                         ___\ \/ /_ __   __ _| |_
                        / _ \\  /| '_ \ / _` | __|
                       |  __//  \| |_) | (_| | |_
                        \___/_/\_\ .__/ \__,_|\__|
                                 |_| XML parser

   Copyright (c) 2019      David Loffredo <loffredo@steptools.com>
   Copyright (c) 2019-2026 Sebastian Pipping <sebastian@pipping.org>
   Copyright (c) 2019      Ben Wagner <bungeman@chromium.org>
   Copyright (c) 2019      Vadim Zeitlin <vadim@zeitlins.org>
   Copyright (c) 2026      Matthew Fernandez <matthew.fernandez@gmail.com>
   Licensed under the MIT license:

   Permission is  hereby granted,  free of charge,  to any  person obtaining
   a  copy  of  this  software   and  associated  documentation  files  (the
   "Software"),  to  deal in  the  Software  without restriction,  including
   without  limitation the  rights  to use,  copy,  modify, merge,  publish,
   distribute, sublicense, and/or sell copies of the Software, and to permit
   persons  to whom  the Software  is  furnished to  do so,  subject to  the
   following conditions:

   The above copyright  notice and this permission notice  shall be included
   in all copies or substantial portions of the Software.

   THE  SOFTWARE  IS  PROVIDED  "AS  IS",  WITHOUT  WARRANTY  OF  ANY  KIND,
   EXPRESS  OR IMPLIED,  INCLUDING  BUT  NOT LIMITED  TO  THE WARRANTIES  OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
   NO EVENT SHALL THE AUTHORS OR  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR  OTHER LIABILITY, WHETHER  IN AN  ACTION OF CONTRACT,  TORT OR
   OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "random_rand_s.h"

/* force stdlib to define rand_s() */
#if ! defined(_CRT_RAND_S)
#  define _CRT_RAND_S
#endif

// Workaround MinGW GCC trouble with recognizing `rand_s`, likely related
// to return type `error_t`; the symptom was:
// > error: implicit declaration of function ‘rand_s’
#if defined(__MINGW32__)
#  include <errno.h>
#endif

#include <stdlib.h> // for rand_s
#include <string.h> // for memcpy

// Help clang-tidy out with prototype of function `rand_s`
#if defined(XML_CLANG_TIDY)
int rand_s(unsigned int *);
#endif

/* Provide declaration of rand_s() for MinGW-32 (not 64, which has it),
   as it didn't declare it in its header prior to version 5.3.0 of its
   runtime package (mingwrt, containing stdlib.h).  The upstream fix
   was introduced at https://osdn.net/projects/mingw/ticket/39658 . */
#if defined(__MINGW32__) && defined(__MINGW32_VERSION)                         \
    && __MINGW32_VERSION < 5003000L && ! defined(__MINGW64_VERSION_MAJOR)
__declspec(dllimport) int rand_s(unsigned int *);
#endif

/* Obtain entropy on Windows using the rand_s() function which
 * generates cryptographically secure random numbers.  Internally it
 * uses RtlGenRandom API which is present in Windows XP and later.
 */
bool
writeRandomBytes_rand_s(void *target, size_t count) {
  size_t bytesWrittenTotal = 0;

  while (bytesWrittenTotal < count) {
    unsigned int random32 = 0;

    if (rand_s(&random32))
      return false; /* failure */

    size_t toUse = count - bytesWrittenTotal;
    if (toUse > sizeof(random32))
      toUse = sizeof(random32);
    memcpy((char *)target + bytesWrittenTotal, &random32, toUse);
    bytesWrittenTotal += toUse;
  }
  return true; /* success */
}
