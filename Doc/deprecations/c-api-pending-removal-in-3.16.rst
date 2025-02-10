Pending removal in Python 3.16
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* C structures for :py:type:`str` instances, and a macro for querying a
  string's memory layout:

  * :c:type:`PyASCIIObject`
  * :c:type:`PyCompactUnicodeObject`
  * :c:type:`PyUnicodeObject`
  * :c:type:`PyUnicode_IS_COMPACT`

  See their documentation for possible replacements.
