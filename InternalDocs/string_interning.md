# String interning

Strings (`PyUnicode_*`) can have the following properties:

- *interned*: For a given string contents, only up to one *interned* string
  exists (interpreter-wide). So, two interned strings can be compared using
  pointer comparison (`is` in Python).
  This is used to optimize dict and attribute lookups, among other things.
- *immortal*: As with other immortal objects: exists until interpreter
  finalization. Reference counting operations don't have any effect,
  and can be safely skipped.
- *singleton*: statically allocated in the core or deep-frozen modules
  (but not extensions). Indicated by the `statically_allocated` flag.
  These are:
  - `_Py_ID` and `_Py_STR` strings collected by `make regen-global-objects`
    (`Tools/build/generate_global_objects.py`); initialazed and interned at
    runtime initialization time, and
  - the 256 length-1 latin-1 strings. These are not interned at init,
    but special-cased.

The intern state is checked with PyUnicode_CHECK_INTERNED(s), and can be:

- `SSTATE_NOT_INTERNED`
- `SSTATE_INTERNED_MORTAL`
- `SSTATE_INTERNED_IMMORTAL`
- `SSTATE_INTERNED_IMMORTAL_STATIC`

Every object starts out as `SSTATE_NOT_INTERNED`, and can be interned using
the following functions:

- `_PyUnicode_InternImmortal`
- `_PyUnicode_InternMortal`
- `_PyUnicode_InternStaticSingleton`

Note that interning a *statically allocated* singleton is *not* guaranteed to
give you a statically allocated object back. (This happens when someone
has interned a dynamically allocated object before your call.)

Note that because the interning state includes info about immortality,
the generic `_Py_SetImmoral`*must not* be used on strings!
Always use `_PyUnicode_InternImmortal` instead.


## Statically allocated strings

All the interning functions delegate to `_PyUnicode_InternStatic` when given
a *statically allocated* string.
Use the function directly if you know you have one.

If an equivalent heap string was interned *before* a call to
`_PyUnicode_InternStatic`, you'll get the previously interned string back.
That means this function does *not* guarantee a *statically allocated* object
(i.e. the interned string might not be shared between interpreters).
However, you are guaranteed to get an *immortal* string.

Statically allocated strings are interned in their own, process-wide lookup
table. However, the interpreter-specific table takes priority; for example:
- interpreter A interns heap string "foo" into its dict
- interpreter B interns static string "foo" into the shared lookup table
- A interns a new string "foo"; it must get the one it interned previously


## Interning with immortalization

The `_PyUnicode_InternImmortal` function interns a string *and* makes the
result immortal. The *only* other way to create an immortal string is
having it *statically allocated*.

As with any immortal object, you should only use this for strings that will
live until interpreter shutdown.
We currently also use in for strings contained in code objects and similar.
These are “close enough” to immortal: even in use cases like hot reloading
or `eval`-ing user input, the number of distinct variable names and string
constants expected to stay low.


## Interning mortal strings

For mortal strings, 2 references from the interned dict (key and value) are
excluded from the object's refcount. When a string is deallocated,

Reference counting: `_PyUnicode_InternMortal` takes ownership of (“steals”)
the reference to its argument, and updates the argument with a new reference.
This makes it “reference neutral”. Beware that the function must not be called
with a borrowed reference.
