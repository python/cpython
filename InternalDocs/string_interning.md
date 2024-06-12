# String interning

*Interned* strings are conceptually part of an interpreter-global
*set* of interned strings, meaning that
- no two interned strings have the same content (across an interpreter)
- two interned strings can be safely compared using pointer equality
  (Python `is`)

This is used to optimize dict and attribute lookups, among other things.

Python uses three different mechanisms to intern strings:

- Singleton strings marked in C source with `_Py_STR` and `_Py_ID` macros.
  These are statically allocated, and collected using `make regen-global-objects`
  (`Tools/build/generate_global_objects.py`), which generates code
  for declaration, initialization and finalization.

  The difference between the two kinds is not important. (A `_Py_ID` string is
  a valid C name, with which we can refer to it; a `_Py_STR` may e.g. contain
  non-identifier characters, so it needs a separate C-compatible name.)

  The empty string is in this category (as `_Py_STR(empty)`).

  These singletons are interned in a runtime-global lookup table,
  `_PyRuntime.cached_objects.interned_strings` (`INTERNED_STRINGS`),
  at runtime initialization.

- The 256 possible one-character latin-1 strings are singletons,
  which can be retrieved with `_Py_LATIN1_CHAR_STRING(c)`, are stored in
  runtime-global arrays, `_PyRuntime.static_objects.strings.ascii` and
  `_PyRuntime.static_objects.strings.latin1`.

  These are NOT interned at startup in the normal build.
  In the free-threaded build, they are; this avoids modifying the
  global lookup table after threads are started.

  Interning a one-byte string will always intern the corresponding singleton.
  (This is the reason to mention these here: they complicate the interning
  mechanism.)

- All other strings are allocated dynamically, and have their
  `_PyUnicode_STATE(s).statically_allocated` flag set to zero.
  When interned, such strings are added to an interpreter-wide dict,
  `PyInterpreterState.cached_objects.interned_strings`.

  The key and value of each entry in this dict reference the same object.

The three sets of singletons (`_Py_STR`, `_Py_ID` and one-byte strings)
are disjoint.
If you have such a singleton, it (and no other copy) will be interned.


## Immortality and reference counting

Invariant: Every immortal string is interned. (Except the one-byte singletons:
those might but might not be interned.)

In practice, this means that you must not use `_Py_SetImmoral` to immortalize
a string, unless you already know that it is interned.

The converse is not true: interned strings can be mortal.
For these mortal interned strings:
- the 2 references from the interned dict (key & value) are excluded from
  their refcount
- the deallocator (`unicode_dealloc`) removes the string from the interned dict
- at shutdown, when the interned dict is cleared, the references are added back

As with any type, you should only immortalize strings that will live until
interpreter shutdown.
We currently also use in for strings contained in code objects and similar:
(specifically in the compiler and in `marshal`).
These are “close enough” to immortal: even in use cases like hot reloading
or `eval`-ing user input, the number of distinct variable names and string
constants expected to stay low.


## State

The intern state is checked with PyUnicode_CHECK_INTERNED(s), and can be:

- `SSTATE_NOT_INTERNED` (zero)
- `SSTATE_INTERNED_MORTAL`
- `SSTATE_INTERNED_IMMORTAL`
- `SSTATE_INTERNED_IMMORTAL_STATIC`


## Internal API

We have the following *internal* API for interning:

- `_PyUnicode_InternMortal`: just intern the string
- `_PyUnicode_InternImmortal`: intern, and immortalize the result
- `_PyUnicode_InternStatic`: intern a static singleton (`_Py_STR`, `_Py_ID`
  or one-byte). Not for general use.

All take an interpreter state, and a pointer to a `PyObject*` which they
modify in place.

The functions take ownership of (“steal”) the reference to their argument,
and update the argument with a *new* reference.
This means:
- They're “reference neutral”.
- They must not be called with a borrowed reference.
