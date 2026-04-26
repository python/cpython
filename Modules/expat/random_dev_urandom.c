/*
                            __  __            _
                         ___\ \/ /_ __   __ _| |_
                        / _ \\  /| '_ \ / _` | __|
                       |  __//  \| |_) | (_| | |_
                        \___/_/\_\ .__/ \__,_|\__|
                                 |_| XML parser

   Copyright (c) 2017-2026 Sebastian Pipping <sebastian@pipping.org>
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

#include "random_dev_urandom.h"

#if ! defined(_POSIX_C_SOURCE)                                                 \
    || (defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE < 200809L))
#  define _POSIX_C_SOURCE 200809L // for O_CLOEXEC
#endif

#include <errno.h>
#include <fcntl.h>  // open
#include <unistd.h> // close

/* Extract entropy from /dev/urandom */
bool
writeRandomBytes_dev_urandom(void *target, size_t count) {
  int success = false; /* full count bytes written? */
  size_t bytesWrittenTotal = 0;

  const int fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
  if (fd < 0) {
    return 0;
  }

  do {
    void *const currentTarget = (void *)((char *)target + bytesWrittenTotal);
    const size_t bytesToWrite = count - bytesWrittenTotal;

    errno = 0;

    const ssize_t bytesWrittenMore = read(fd, currentTarget, bytesToWrite);

    if (bytesWrittenMore > 0) {
      bytesWrittenTotal += bytesWrittenMore;
      if (bytesWrittenTotal >= count)
        success = true;
    }
  } while (! success && (errno == EINTR));

  close(fd);
  return success;
}
