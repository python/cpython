# String interning

Strings (`PyUnicode_*`) can have the following properties:

- *interned*: For a given string contents, only up to one *interned* string
  exists (interpreter-wide). So, two interned strings can be compared using
  pointer comparison (`is` in Python).
  This is used to optimize dict and attribute lookups, among other things.
- *immortal*: As with other immortal objects: exists until interpreter
  finalization. Reference counting operations don't have any effect,
  and can be safely skipped.
- *statically allocated*: `static`. Shared between interpreters.

The intern state is checked with PyUnicode_CHECK_INTERNED(s), and can:

- `SSTATE_NOT_INTERNED`
- `SSTATE_INTERNED_MORTAL`
- `SSTATE_INTERNED_IMMORTAL`
- `SSTATE_INTERNED_IMMORTAL_STATIC`

Every object starts out as `SSTATE_NOT_INTERNED`, and can be interned using
the following functions:

- `_PyUnicode_InternStatic`
- `_PyUnicode_InternImmortal`
- `_PyUnicode_InternMortal`

All of these intern an object in place. That is: Before the call you should own
a strong reference to `*p`, and after the call you still own a reference
to `*p` but it might not be the same object.

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

`_PyUnicode_InternStatic` is guaranteed to give you an *immortal* string.
However, it does *not* guarantee a *statically allocated* object.
(Someone might have interned an equivalent heap string before you!)


## Interning with immortalization

The `_PyUnicode_InternImmortal` function interns a string *and* makes the
result immortal. The *only* other way to create an immortal string is
having it *statically allocated*.

As with any immortal object, you should only use this for strings that will
live until interpreter shutdown.
We currently also use in for strings contained in code objects and similar;
these are “close enough” to immortal.
