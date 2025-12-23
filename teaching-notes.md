# Teaching Notes (For Claude - Not for Student)

This file contains detailed research and implementation guidance for teaching CPython internals through building a Record type.

---

## Phase 1: PyObject Fundamentals

### PyObject Structure
**File:** `Include/object.h:105-109`
```c
typedef struct _object {
    _PyObject_HEAD_EXTRA
    Py_ssize_t ob_refcnt;
    PyTypeObject *ob_type;
} PyObject;
```

### PyVarObject Structure
**File:** `Include/object.h:115-118`
```c
typedef struct {
    PyObject ob_base;
    Py_ssize_t ob_size;
} PyVarObject;
```

### Py_INCREF/DECREF
**File:** `Include/object.h:461-508`
- INCREF: Simply `op->ob_refcnt++`
- DECREF: Decrements, calls `_Py_Dealloc(op)` when reaches 0
- Debug builds track `_Py_RefTotal` and detect negative refcounts

### Teaching Questions - Answers

**Q: What are the two fields every Python object has?**
A: `ob_refcnt` (reference count) and `ob_type` (pointer to type object)

**Q: Why reference counting instead of tracing GC?**
A: Deterministic destruction (know exactly when objects die), simpler implementation, good cache locality. Downside: can't handle cycles (hence the cycle collector supplement).

**Q: Should Record use PyObject or PyVarObject?**
A: PyVarObject - because Record has variable number of fields. The `ob_size` will store field count.

---

## Phase 2: Data Structures for Record

### Tuple as Reference
**File:** `Include/cpython/tupleobject.h:5-11`
```c
typedef struct {
    PyObject_VAR_HEAD
    PyObject *ob_item[1];    // Flexible array member
} PyTupleObject;
```

### Record Memory Layout Design
```c
typedef struct {
    PyObject_VAR_HEAD           // includes ob_size = field count
    Py_hash_t r_hash;           // cached hash (-1 if not computed)
    PyObject *r_names;          // tuple of field names (strings)
    PyObject *r_values[1];      // flexible array of values
} RecordObject;
```

**Design decisions:**
- `r_names` is a tuple (shared across records with same fields)
- `r_values` is inline for cache locality
- `r_hash` cached because immutable (like tuple)
- Use `ob_size` for field count

**Q: Tradeoff tuple vs dict for names?**
A: Tuple is simpler and faster for small N. Dict would be O(1) lookup but more memory. For typical record sizes (2-10 fields), linear scan of tuple is fine. Could optimize with dict for large records.

### Key Functions to Study
- `Objects/tupleobject.c:tuple_hash` (line ~350) - hash combining algorithm
- `Objects/tupleobject.c:tuple_richcompare` (line ~600) - element-by-element comparison
- `Objects/tupleobject.c:tuple_dealloc` (line ~250) - DECREF each element

---

## Phase 3: Type Slots Implementation

### Slot Reference Table

| Slot | Signature | Purpose |
|------|-----------|---------|
| `tp_dealloc` | `void (*)(PyObject *)` | Release resources |
| `tp_repr` | `PyObject *(*)(PyObject *)` | `repr(obj)` |
| `tp_hash` | `Py_hash_t (*)(PyObject *)` | `hash(obj)` |
| `tp_richcompare` | `PyObject *(*)(PyObject *, PyObject *, int)` | Comparisons |
| `tp_getattro` | `PyObject *(*)(PyObject *, PyObject *)` | `obj.attr` |
| `sq_length` | `Py_ssize_t (*)(PyObject *)` | `len(obj)` |
| `sq_item` | `PyObject *(*)(PyObject *, Py_ssize_t)` | `obj[i]` |

### Implementation Patterns

**record_dealloc:**
```c
static void
record_dealloc(RecordObject *r)
{
    Py_ssize_t i, n = Py_SIZE(r);
    PyObject_GC_UnTrack(r);  // If GC tracked
    Py_XDECREF(r->r_names);
    for (i = 0; i < n; i++) {
        Py_XDECREF(r->r_values[i]);
    }
    Py_TYPE(r)->tp_free((PyObject *)r);
}
```

**record_hash (based on tuple_hash):**
```c
static Py_hash_t
record_hash(RecordObject *r)
{
    if (r->r_hash != -1)
        return r->r_hash;

    Py_hash_t hash = 0x345678L;
    Py_ssize_t n = Py_SIZE(r);
    Py_hash_t mult = 1000003L;

    for (Py_ssize_t i = 0; i < n; i++) {
        Py_hash_t h = PyObject_Hash(r->r_values[i]);
        if (h == -1) return -1;
        hash = (hash ^ h) * mult;
        mult += 82520L + n + n;
    }
    hash += 97531L;
    if (hash == -1)
        hash = -2;
    r->r_hash = hash;
    return hash;
}
```

**record_getattro:**
```c
static PyObject *
record_getattro(RecordObject *r, PyObject *name)
{
    // First check if it's a field name
    if (PyUnicode_Check(name)) {
        Py_ssize_t n = Py_SIZE(r);
        for (Py_ssize_t i = 0; i < n; i++) {
            PyObject *field = PyTuple_GET_ITEM(r->r_names, i);
            if (PyUnicode_Compare(name, field) == 0) {
                PyObject *val = r->r_values[i];
                Py_INCREF(val);
                return val;
            }
        }
    }
    // Fall back to generic attribute lookup (for methods, etc.)
    return PyObject_GenericGetAttr((PyObject *)r, name);
}
```

### Common Pitfalls
1. Forgetting to INCREF return values from sq_item/getattro
2. Not handling negative indices in sq_item
3. Forgetting to handle hash == -1 (error indicator)
4. Not calling PyObject_GC_UnTrack in dealloc if type is GC-tracked

---

## Phase 4: Evaluation Loop

### Key Locations
- `_PyEval_EvalFrameDefault`: `Python/ceval.c:1577`
- Stack macros: `Python/ceval.c:1391-1433`
- `BUILD_TUPLE`: `Python/ceval.c:2615-2630`
- `BUILD_MAP`: `Python/ceval.c:2648-2700`

### BUILD_TUPLE Pattern
```c
case TARGET(BUILD_TUPLE): {
    PyObject *tup = PyTuple_New(oparg);
    if (tup == NULL)
        goto error;
    while (--oparg >= 0) {
        PyObject *item = POP();
        PyTuple_SET_ITEM(tup, oparg, item);
    }
    PUSH(tup);
    DISPATCH();
}
```

### BUILD_RECORD Design
Stack layout: `[..., name1, val1, name2, val2, ..., nameN, valN]`
Oparg: N (number of fields)
Pops: 2*N items
Pushes: 1 record

```c
case TARGET(BUILD_RECORD): {
    Py_ssize_t n = oparg;
    PyObject *names = PyTuple_New(n);
    if (names == NULL)
        goto error;

    // Collect names and values from stack (reverse order)
    PyObject **values = PyMem_Malloc(n * sizeof(PyObject *));
    if (values == NULL) {
        Py_DECREF(names);
        goto error;
    }

    for (Py_ssize_t i = n - 1; i >= 0; i--) {
        PyObject *val = POP();
        PyObject *name = POP();
        if (!PyUnicode_Check(name)) {
            // Error: field name must be string
            // cleanup and goto error
        }
        PyTuple_SET_ITEM(names, i, name);  // steals ref
        values[i] = val;  // we own this ref
    }

    PyObject *record = PyRecord_New(names, values, n);
    PyMem_Free(values);
    if (record == NULL) {
        Py_DECREF(names);
        goto error;
    }

    PUSH(record);
    DISPATCH();
}
```

---

## Phase 5: Implementation Details

### File Structure

**Include/recordobject.h:**
```c
#ifndef Py_RECORDOBJECT_H
#define Py_RECORDOBJECT_H

#include "Python.h"

typedef struct {
    PyObject_VAR_HEAD
    Py_hash_t r_hash;
    PyObject *r_names;
    PyObject *r_values[1];
} RecordObject;

PyAPI_DATA(PyTypeObject) PyRecord_Type;

#define PyRecord_Check(op) PyObject_TypeCheck(op, &PyRecord_Type)

PyAPI_FUNC(PyObject *) PyRecord_New(PyObject *names, PyObject **values, Py_ssize_t n);

#endif
```

### Build Integration

**Makefile.pre.in additions:**
```makefile
OBJECT_OBJS= \
    ... \
    Objects/recordobject.o
```

**Python/bltinmodule.c:**
Add to `_PyBuiltin_Init`:
```c
SETBUILTIN("Record", &PyRecord_Type);
```

### Opcode Number Selection
Check `Lib/opcode.py` for gaps. In 3.10:
- Gap at 35-48
- Gap at 58
- Could use 35 for BUILD_RECORD

Add to `Lib/opcode.py`:
```python
def_op('BUILD_RECORD', 35)
hasconst.append(35)  # or maybe not, depends on design
```

---

## Teaching Strategies

### Phase 1 Approach
1. Have student grep for "typedef struct _object"
2. Ask them to explain each field before revealing
3. Demo with: `import sys; x = []; print(sys.getrefcount(x))`
4. Show how INCREF/DECREF work with print statements in debug build

### Phase 2 Approach
1. Compare tuple and list side by side - why different structures?
2. Have student sketch Record layout before showing solution
3. Discuss space/time tradeoffs

### Phase 3 Approach
1. Start with dealloc - "what happens when refcount hits 0?"
2. Work through repr next - visible feedback
3. Leave hash for after they understand the algorithm from tuple

### Phase 4 Approach
1. Use GDB to step through BUILD_TUPLE
2. Print stack_pointer values before/after
3. Have student write pseudocode before C

### Phase 5 Approach
1. Start with minimal working type (just dealloc + repr)
2. Add features incrementally, test each
3. Opcode last, after type fully works

---

## Quick Reference: Key Line Numbers (3.10)

| Item | File | Line |
|------|------|------|
| PyObject | Include/object.h | 105 |
| PyVarObject | Include/object.h | 115 |
| Py_INCREF | Include/object.h | 461 |
| Py_DECREF | Include/object.h | 477 |
| PyTypeObject | Include/cpython/object.h | 191 |
| PyTupleObject | Include/cpython/tupleobject.h | 5 |
| tuple_hash | Objects/tupleobject.c | ~350 |
| tuple_richcompare | Objects/tupleobject.c | ~600 |
| tuple_dealloc | Objects/tupleobject.c | ~250 |
| PyTuple_Type | Objects/tupleobject.c | ~750 |
| _PyEval_EvalFrameDefault | Python/ceval.c | 1577 |
| Stack macros | Python/ceval.c | 1391 |
| BUILD_TUPLE | Python/ceval.c | 2615 |
| BUILD_MAP | Python/ceval.c | 2648 |

---

## Solution Branch Reference

The `teaching-cpython-solution` branch contains:
- `Include/recordobject.h` - Complete header
- `Objects/recordobject.c` - Full implementation (~490 lines)
- Modified `Lib/opcode.py` - BUILD_RECORD opcode 166
- Modified `Python/ceval.c` - BUILD_RECORD handler
- Modified `Makefile.pre.in` - Build integration
- Modified `Objects/object.c` - Type initialization
- Modified `Python/bltinmodule.c` - Builtin registration

Use this as reference when student gets stuck, but guide them to discover solutions themselves first.

---

## Lessons Learned During Implementation

These are practical issues encountered during actual implementation:

### 1. Hash Constants Not Publicly Exposed

The XXH3-style hash constants (`_PyHASH_XXPRIME_1`, `_PyHASH_XXPRIME_2`, `_PyHASH_XXPRIME_5`) used by `tupleobject.c` are defined locally in that file, not in a header. Had to copy them into `recordobject.c`:

```c
#if SIZEOF_PY_UHASH_T > 4
#define _PyHASH_XXPRIME_1 ((Py_uhash_t)11400714785074694791ULL)
#define _PyHASH_XXPRIME_2 ((Py_uhash_t)14029467366897019727ULL)
#define _PyHASH_XXPRIME_5 ((Py_uhash_t)2870177450012600261ULL)
#define _PyHASH_XXROTATE(x) ((x << 31) | (x >> 33))
#else
#define _PyHASH_XXPRIME_1 ((Py_uhash_t)2654435761UL)
#define _PyHASH_XXPRIME_2 ((Py_uhash_t)2246822519UL)
#define _PyHASH_XXPRIME_5 ((Py_uhash_t)374761393UL)
#define _PyHASH_XXROTATE(x) ((x << 13) | (x >> 19))
#endif
```

**Teaching point:** Internal implementation details are often not exposed. Look at how existing code solves the same problem.

### 2. GC Trashcan for Deep Recursion

Deallocation needs `Py_TRASHCAN_BEGIN`/`Py_TRASHCAN_END` to prevent stack overflow when destroying deeply nested structures:

```c
static void
record_dealloc(RecordObject *r)
{
    PyObject_GC_UnTrack(r);
    Py_TRASHCAN_BEGIN(r, record_dealloc)

    // ... cleanup code ...

    Py_TRASHCAN_END
}
```

**Teaching point:** Check how `tuple_dealloc` and `list_dealloc` handle this.

### 3. PyUnicode_Compare Error Handling

`PyUnicode_Compare` can raise exceptions (e.g., if comparison fails). Must check `PyErr_Occurred()`:

```c
int cmp = PyUnicode_Compare(name, field);
if (cmp == 0) {
    // found match
}
if (PyErr_Occurred()) {
    return NULL;
}
```

### 4. Opcode Number Selection

Originally planned opcode 35, but ended up using 166 (right after `DICT_UPDATE` at 165). The gaps in the opcode table exist for historical reasons and some are reserved.

**Teaching point:** Look at recent additions to see where new opcodes go. `Lib/opcode.py` is the source of truth.

### 5. Python-Level Constructor (tp_new)

To test the Record type without emitting bytecode, need a Python constructor:

```c
static PyObject *
record_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    // Record() only accepts keyword arguments
    if (PyTuple_GET_SIZE(args) != 0) {
        PyErr_SetString(PyExc_TypeError,
                        "Record() takes no positional arguments");
        return NULL;
    }
    // ... iterate kwds, build names tuple and values array
}
```

This allows: `Record(x=10, y=20)` without needing compiler changes.

### 6. Type Initialization Order

The type must be initialized in `_PyTypes_Init()` in `Objects/object.c`:

```c
INIT_TYPE(PyRecord_Type);
```

And exposed as builtin in `Python/bltinmodule.c`:

```c
SETBUILTIN("Record", &PyRecord_Type);
```

### 7. Correct Allocator for GC-Tracked VarObjects

Use `PyObject_GC_NewVar` for variable-size objects that participate in GC:

```c
record = PyObject_GC_NewVar(RecordObject, &PyRecord_Type, n);
```

Not `PyObject_NewVar` (which doesn't set up GC tracking).

### 8. tp_basicsize Calculation

For variable-size objects, `tp_basicsize` should be the size WITHOUT the variable part:

```c
sizeof(RecordObject) - sizeof(PyObject *)  /* tp_basicsize */
sizeof(PyObject *)                          /* tp_itemsize */
```

The `- sizeof(PyObject *)` accounts for the `r_values[1]` flexible array member.

---

## Testing Checklist

After implementation, verify:

```python
# Basic construction
r = Record(x=10, y=20)

# repr
assert repr(r) == "Record(x=10, y=20)"

# Attribute access
assert r.x == 10
assert r.y == 20

# Indexing
assert r[0] == 10
assert r[-1] == 20
assert len(r) == 2

# Hashing (usable as dict key)
d = {r: "value"}
r2 = Record(x=10, y=20)
assert d[r2] == "value"

# Equality
assert r == r2
assert r != Record(x=10, y=30)
assert r != Record(a=10, b=20)  # different names

# Error handling
try:
    r[5]
except IndexError:
    pass

try:
    Record()  # no kwargs
except TypeError:
    pass

try:
    Record(1, 2, 3)  # positional args
except TypeError:
    pass
```
