/*
                            __  __            _
                         ___\ \/ /_ __   __ _| |_
                        / _ \\  /| '_ \ / _` | __|
                       |  __//  \| |_) | (_| | |_
                        \___/_/\_\ .__/ \__,_|\__|
                                 |_| XML parser

   Copyright (c) 2017-2026 Sebastian Pipping <sebastian@pipping.org>
   Copyright (c) 2017      Chanho Park <chanho61.park@samsung.com>
   Copyright (c) 2022      Sean McBride <sean@rogue-research.com>
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

#include "random_getrandom.h"

#include "expat_config.h" // for HAVE_GETRANDOM, HAVE_SYSCALL_GETRANDOM

#if defined(HAVE_GETRANDOM)
#  include <sys/random.h> /* getrandom */
#endif

#if defined(HAVE_SYSCALL_GETRANDOM)
#  if ! defined(_GNU_SOURCE)
#    define _GNU_SOURCE 1 /* syscall prototype */
#  endif
#  include <unistd.h>      /* syscall */
#  include <sys/syscall.h> /* SYS_getrandom */
#endif                     // defined(HAVE_SYSCALL_GETRANDOM)

#if ! defined(GRND_NONBLOCK)
#  define GRND_NONBLOCK 0x0001
#endif /* defined(GRND_NONBLOCK) */

#include <assert.h>
#include <errno.h>
#include <limits.h> // for INT_MAX

/* Obtain entropy on Linux 3.17+ */
bool
writeRandomBytes_getrandom_nonblock(void *target, size_t count) {
  int success = false; /* full count bytes written? */
  size_t bytesWrittenTotal = 0;
  const unsigned int getrandomFlags = GRND_NONBLOCK;

  do {
    void *const currentTarget = (void *)((char *)target + bytesWrittenTotal);
    const size_t bytesToWrite = count - bytesWrittenTotal;

    assert(bytesToWrite <= INT_MAX);

    errno = 0;

    const int bytesWrittenMore =
#if defined(HAVE_GETRANDOM)
        (int)getrandom(currentTarget, bytesToWrite, getrandomFlags);
#else
        (int)syscall(SYS_getrandom, currentTarget, bytesToWrite,
                     getrandomFlags);
#endif

    if (bytesWrittenMore > 0) {
      bytesWrittenTotal += bytesWrittenMore;
      if (bytesWrittenTotal >= count)
        success = true;
    }
  } while (! success && (errno == EINTR));

  return success;
}
