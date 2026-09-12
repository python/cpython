/* unicodedata property-index interface for the re module's \p{...} matcher.

   The capsule exposes the value index that unicodedata stores for each
   character property (the same index returned by unicodedata._ucd_re_info()),
   so the SRE engine can match \p{...} in C without a per-character Python call
   into unicodedata.  See Lib/re/_properties.py and Modules/_sre/sre.c. */

#ifndef Py_INTERNAL_UNICODEDATA_RE_H
#define Py_INTERNAL_UNICODEDATA_RE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#define PyUnicodeData_RE_CAPSULE_NAME "unicodedata._ucd_re_CAPI"

/* Property selectors.  Private to unicodedata and _sre; the numbering only
   needs to be consistent within a single build. */
enum {
    _Py_UCD_RE_BC = 0,        /* Bidi_Class */
    _Py_UCD_RE_EA,            /* East_Asian_Width */
    _Py_UCD_RE_GCB,           /* Grapheme_Cluster_Break */
    _Py_UCD_RE_INCB,          /* Indic_Conjunct_Break */
};

typedef struct {
    /* Return the value index of property prop for character ch, matching the
       indices in unicodedata._ucd_re_info(); -1 for an unknown property. */
    int (*property)(int prop, Py_UCS4 ch);
} _PyUnicode_RE_CAPI;

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_UNICODEDATA_RE_H */
