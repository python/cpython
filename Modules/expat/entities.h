/* Internal helpers related to entities
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

#if ! defined(EXPAT_ENTITIES_H)
#  define EXPAT_ENTITIES_H 1

#  include "expat_config.h"   // for XML_GE
#  include "expat_external.h" // for XML_Char
#  include "internal.h"       // for XML_NONTESTING_STATIC
#  include <stdbool.h>        // for size_t
#  include <stddef.h>         // for size_t

typedef struct {
  const XML_Char *name;
  const XML_Char *textPtr;
  size_t textLen;   /* length in XML_Chars */
  size_t processed; /* # of processed bytes - when suspended */
  const XML_Char *systemId;
  const XML_Char *base;
  const XML_Char *publicId;
  const XML_Char *notation;
  bool open;
  bool hasMore; /* true if entity has not been completely processed */
  /* An entity can be open while being already completely processed (!hasMore).
    The reason is the delayed closing of entities until their inner
    entities are processed and closed */
  bool is_param;
  bool is_internal; /* true if declared in internal subset outside PE */
#  if XML_GE == 1
  /* Indirect accounting bytes charged when this entity is expanded: 1 per text
     character, plus each reference's "&name;"/"%name;" token length and its own
     expansion. Matches XML_ACCOUNT_ENTITY_EXPANSION, so it is not the
     replacement-text size. 0 means "not computed yet" (an empty entity is 0
     too, told apart by textLen == 0). */
  XmlBigCount expandedSize;
  bool beingAnalyzed; /* on the resolveEntityClosureExpandedSize depth-first
                             search stack; used for cycle detection */
  const XML_Char
      *scanPtr; /* cursor position into the entity's replacement text; NULL when
                   `textPtr` is NULL, else a pointer into the entity value,
                   ranging from `.textPtr` to `.textPtr + .textLen`. */
#  endif        /* XML_GE == 1 */
} ENTITY;

#  if XML_GE == 1
/* How a scanEntityText call ended. */
enum EntityScanResult {
  ENTITY_SCAN_COMPLETE, /* reached end of text; *expandedSize is final */
  ENTITY_SCAN_BLOCKED,  /* stopped at a defined entity whose expanded
                           size has not been computed yet; *blockerOut
                           names the entity whose size to compute
                           first */
  ENTITY_SCAN_OVERFLOW  /* the expanded size would exceed XmlBigCount;
                           callers must abort parsing */
};

XML_NONTESTING_STATIC int isCDataCommentOrPIStart(const XML_Char *pos,
                                                  size_t maxLen,
                                                  size_t *openTextLen);

XML_NONTESTING_STATIC int isPredefinedEntityName(const XML_Char *name,
                                                 size_t nameLen);

XML_NONTESTING_STATIC enum EntityScanResult
scanEntityText(XML_Parser parser, ENTITY *entity, const XML_Char **posPtr,
               XmlBigCount *expandedSize, ENTITY **blockerOut);
#  endif /* XML_GE == 1 */

#endif // ! defined(EXPAT_ENTITIES_H)
