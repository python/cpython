/* Hash table related internal API
                            __  __            _
                         ___\ \/ /_ __   __ _| |_
                        / _ \\  /| '_ \ / _` | __|
                       |  __//  \| |_) | (_| | |_
                        \___/_/\_\ .__/ \__,_|\__|
                                 |_| XML parser

   Copyright (c) 2026 Sebastian Pipping <sebastian@pipping.org>
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

#if ! defined(HASH_TABLE_H)
#  define HASH_TABLE_H 1

#  include "expat.h"    // for XML_Bool, XML_Parser
#  include "internal.h" // for XML_NONTESTING_STATIC

#  include <stddef.h> // for size_t

typedef const XML_Char *KEY;

typedef struct {
  KEY name;
} NAMED;

typedef struct {
  NAMED **v;
  unsigned char power;
  size_t size;
  size_t used;
  XML_Parser parser;
} HASH_TABLE;

typedef struct {
  NAMED **p;
  NAMED **end;
} HASH_TABLE_ITER;

XML_NONTESTING_STATIC NAMED *lookupWithLength(XML_Parser parser,
                                              HASH_TABLE *table, KEY name,
                                              size_t nameLen,
                                              size_t createSize);
XML_NONTESTING_STATIC NAMED *lookup(XML_Parser parser, HASH_TABLE *table,
                                    KEY name, size_t createSize);

XML_NONTESTING_STATIC void hashTableInit(HASH_TABLE *table, XML_Parser parser);
XML_NONTESTING_STATIC void hashTableClear(HASH_TABLE *table);
XML_NONTESTING_STATIC void hashTableDestroy(HASH_TABLE *table);
XML_NONTESTING_STATIC void hashTableIterInit(HASH_TABLE_ITER *iter,
                                             const HASH_TABLE *table);
XML_NONTESTING_STATIC NAMED *hashTableIterNext(HASH_TABLE_ITER *iter);

XML_NONTESTING_STATIC XML_Bool keyeq(KEY s1, size_t s1len, KEY s2);
XML_NONTESTING_STATIC size_t keylen(KEY s);

#endif // ! defined(HASH_TABLE_H)
