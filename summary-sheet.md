# CPython Internals Summary Sheet

## Object Model

```
PyObject (fixed size)          PyVarObject (variable size)
┌─────────────────┐            ┌─────────────────┐
│ ob_refcnt       │            │ ob_refcnt       │
│ ob_type ────────┼──→ type    │ ob_type         │
└─────────────────┘            │ ob_size         │ ← element count
                               ├─────────────────┤
                               │ items[0]        │
                               │ items[1]        │ ← flexible array
                               │ ...             │
                               └─────────────────┘
```

## Reference Counting

```
Py_INCREF(obj)    →  obj->ob_refcnt++
Py_DECREF(obj)    →  obj->ob_refcnt--; if (0) tp_dealloc(obj)
```

Rules:
- INCREF when you store a reference
- DECREF when you release it
- INCREF before returning a PyObject* (caller owns it)
- Forget INCREF → use-after-free
- Forget DECREF → memory leak

Debug builds track `_Py_RefTotal` for leak detection.

## Type Slots

```
obj->ob_type->tp_hash(obj)                    # hash(obj)
obj->ob_type->tp_repr(obj)                    # repr(obj)
obj->ob_type->tp_richcompare(obj, other, op)  # obj == other
obj->ob_type->tp_getattro(obj, name)          # obj.name
obj->ob_type->tp_as_sequence->sq_length(obj)  # len(obj)
obj->ob_type->tp_as_sequence->sq_item(obj, i) # obj[i]
```

NULL slot → TypeError: type doesn't support operation

Prefixes: `tp_` (type), `sq_` (sequence), `nb_` (number), `mp_` (mapping)

## RecordObject Design

```
┌─────────────────┐
│ ob_refcnt       │
│ ob_type         │
│ ob_size = n     │  ← field count
├─────────────────┤
│ names ──────────┼──→ ("x", "y", "z")  ← shared tuple
├─────────────────┤
│ values[0]       │
│ values[1]       │  ← flexible array
│ values[2]       │
└─────────────────┘
```

```c
typedef struct {
    PyObject_VAR_HEAD
    PyObject *names;      // tuple of field names
    PyObject *values[1];  // flexible array
} RecordObject;
```

## Slots to Implement

| Slot | Purpose | Key Pattern |
|------|---------|-------------|
| `tp_dealloc` | Destructor | DECREF names + each value, then tp_free |
| `tp_repr` | `repr(r)` | PyUnicodeWriter, PyObject_Repr per value |
| `tp_hash` | `hash(r)` | Combine element hashes (xxHash), -1 → -2 |
| `tp_richcompare` | `r == r2` | Compare names AND values, Py_NotImplemented for < > |
| `tp_getattro` | `r.x` | Search names, return values[i], fallback GenericGetAttr |
| `tp_new` | `Record(x=1)` | Parse kwargs, call PyRecord_New |
| `sq_length` | `len(r)` | Return Py_SIZE(self) |
| `sq_item` | `r[i]` | Bounds check, INCREF, return |

## Evaluation Loop

```
_PyEval_EvalFrameDefault() {
    for (;;) {
        switch (opcode) {
            case BUILD_TUPLE: ...
            case BUILD_RECORD: ...  ← we add this
        }
    }
}
```

Stack macros:
- `POP()` - pop and take ownership
- `PUSH(obj)` - push onto stack
- `PEEK(n)` - read without popping
- `STACK_SHRINK(n)` - drop n items

## BUILD_RECORD Design

```
Stack before:  [..., names_tuple, val0, val1, val2]
                     ↑            └─── oparg=3 ───┘
                     └── PEEK(oparg+1)

Stack after:   [..., record]
```

```c
case TARGET(BUILD_RECORD): {
    PyObject *names = PEEK(oparg + 1);
    PyObject **values = &PEEK(oparg);
    PyObject *rec = PyRecord_New(names, values, oparg);  // steals refs
    if (rec == NULL) goto error;
    STACK_SHRINK(oparg + 1);
    PUSH(rec);
    DISPATCH();
}
```

## Reference Stealing vs Copying

```
BUILD_TUPLE:  POP → SET_ITEM (steals)     → no DECREF needed
BUILD_MAP:    PEEK → SetItem (copies)     → must DECREF after
BUILD_RECORD: PEEK → PyRecord_New (steals) → no DECREF needed
```

## Constructors

```
PyRecord_New(names, values, n)     ← C API, called by opcode, steals refs
record_new(type, args, kwargs)     ← Python API (tp_new), parses kwargs
```

## Files to Create/Modify

```
Include/recordobject.h   ← struct + declarations
Objects/recordobject.c   ← implementation
Python/ceval.c           ← BUILD_RECORD case
Lib/opcode.py            ← register opcode number
Makefile.pre.in          ← add to build
Python/bltinmodule.c     ← register type
```
