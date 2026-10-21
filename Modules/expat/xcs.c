/*
                            __  __            _
                         ___\ \/ /_ __   __ _| |_
                        / _ \\  /| '_ \ / _` | __|
                       |  __//  \| |_) | (_| | |_
                        \___/_/\_\ .__/ \__,_|\__|
                                 |_| XML parser

   Copyright (c) 2022-2026 Sebastian Pipping <sebastian@pipping.org>
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

   SPDX-License-Identifier: MIT
*/

#include "xcs.h"

#if defined(XML_UNICODE)
#  if defined(XML_UNICODE_WCHAR_T)
#    include <wchar.h> // for wcscmp, wcslen, wcsncmp
#  endif
#else
#  include <string.h> // for strcmp, strlen, strncmp
#endif

size_t
xcslen(const XML_Char *s) {
#ifdef XML_UNICODE
#  ifdef XML_UNICODE_WCHAR_T
  return wcslen(s);
#  else
  // XML_Char is unsigned short
  size_t len = 0;
  while (s[len]) {
    len++;
  }
  return len;
#  endif
#else
  return strlen(s);
#endif
}

int
xcscmp(const XML_Char *a, const XML_Char *b) {
#if defined(XML_UNICODE)
#  if defined(XML_UNICODE_WCHAR_T)
  return wcscmp(a, b);
#  else
  for (; a[0] && b[0] && a[0] == b[0]; a++, b++)
    ;
  return a[0] - b[0];
#  endif
#else
  return strcmp(a, b);
#endif
}

int
xcsncmp(const XML_Char *a, const XML_Char *b, size_t len) {
#if defined(XML_UNICODE)
#  if defined(XML_UNICODE_WCHAR_T)
  return wcsncmp(a, b, len);
#  else
  for (; len > 0 && a[0] && b[0] && a[0] == b[0]; len--, a++, b++) {
  }
  return (len == 0) ? 0 : (a[0] - b[0]);
#  endif
#else
  return strncmp(a, b, len);
#endif
}
