# Stack references (`_PyStackRef`)

Stack references are the interpreter's tagged representation of values on the evaluation stack.
They carry metadata to track ownership and support optimizations such as tagged small ints.

## Shape and tagging

- A `_PyStackRef` is a tagged pointer-sized value (see `Include/internal/pycore_stackref.h`).
- Tag bits distinguish three cases:
  - `Py_TAG_REFCNT` unset - reference count lives on the pointed-to object.
  - `Py_TAG_REFCNT` set - ownership is "borrowed" (no refcount to drop on close) or the object is immortal.
  - `Py_INT_TAG` set - tagged small integer stored directly in the stackref (no heap allocation).
- Special constants: `PyStackRef_NULL`, `PyStackRef_ERROR`, and embedded `None`/`True`/`False`.

In GIL builds, most objects carry their refcount; tagged borrowed refs skip decref on close. In free
threading builds, the tag is also used to mark deferred refcounted objects so the GC can see them and
to avoid refcount contention on commonly shared objects.

## Converting to and from PyObject*

Three conversions control ownership:

- `PyStackRef_FromPyObjectNew(obj)` - create a new reference (INCREF if mortal).
- `PyStackRef_FromPyObjectSteal(obj)` - take over ownership without changing the count unless the
  object is immortal.
- `PyStackRef_FromPyObjectBorrow(obj)` - create a borrowed stackref (never decref on close).

The `obj` argument must not be `NULL`.

Going back to `PyObject*` mirrors this:

- `PyStackRef_AsPyObjectBorrow(ref)` - borrow the underlying pointer
- `PyStackRef_AsPyObjectSteal(ref)` - transfer ownership from the stackref; if ref is borrowed or
   deferred, this creates a new owning `PyObject*` reference.
- `PyStackRef_AsPyObjectNew(ref)` - create a new owning reference

Only `PyStackRef_AsPyObjectBorrow` allows ref to be `PyStackRef_NULL`.

## Operations on stackrefs

The interpreter treats `_PyStackRef` as the unit of stack storage. Ownership must be managed with
the stackref primitives:

- `PyStackRef_DUP` - like `Py_NewRef` for stackrefs; preserves the original.
- `PyStackRef_Borrow` - create a borrowed stackref from another stackref.
- `PyStackRef_CLOSE` / `PyStackRef_XCLOSE` - like `Py_DECREF`; invalidates the stackref.
- `PyStackRef_CLEAR` - like `Py_CLEAR`; closes and sets the stackref to `PyStackRef_NULL`
- `PyStackRef_MakeHeapSafe` - converts borrowed reference to owning reference

Borrow tracking (for debug builds with `Py_STACKREF_DEBUG`) records who you borrowed from and reports
double-close, leaked borrows, or use-after-close via fatal errors.

## Borrow-friendly opcodes

The interpreter can push borrowed references directly. For example, `LOAD_FAST_BORROW` loads a local
variable as a borrowed `_PyStackRef`, avoiding both INCREF and DECREF for the temporary lifetime on
the evaluation stack.

## Tagged integers on the stack

Small ints can be stored inline with `Py_INT_TAG`, so no heap object is involved. Helpers like
`PyStackRef_TagInt`, `PyStackRef_UntagInt`, and `PyStackRef_IncrementTaggedIntNoOverflow` operate on
these values. Type checks use `PyStackRef_IsTaggedInt` and `PyStackRef_LongCheck`.

## Free threading considerations

With `Py_GIL_DISABLED`, `Py_TAG_DEFERRED` is an alias for `Py_TAG_REFCNT`.
Objects that support deferred reference counting can be pushed to the evaluation
stack and stored in local variables without directly incrementing the reference
count because they are only freed during cyclic garbage collection. This avoids
reference count contention on commonly shared objects such as methods and types. The GC
scans each thread's locals and evaluation stack to keep objects that use
deferred reference counting alive.

## Debugging support

`Py_STACKREF_DEBUG` builds replace the inline tags with table-backed IDs so the runtime can track
creation sites, borrows, closes, and leaks. Enabling `Py_STACKREF_CLOSE_DEBUG` additionally records
double closes. The tables live on `PyInterpreterState` and are initialized in `pystate.c`; helper
routines reside in `Python/stackrefs.c`.
