# CPython Internals Learning Path

A structured curriculum for understanding CPython's implementation by building a custom `Record` type and `BUILD_RECORD` opcode.

## The Project

Build a lightweight immutable record type (like a simplified namedtuple):

```python
r = Record(x=10, y=20, name="point")
r.x        # 10 (attribute access)
r[0]       # 10 (sequence protocol)
len(r)     # 3
hash(r)    # hashable (can be dict key)
repr(r)    # Record(x=10, y=20, name='point')
r == r2    # comparable
```

---

## Phase 1: The Object Model

### 1.1 PyObject - The Universal Base
- [ ] Read `Include/object.h` - find `PyObject` struct (around line 105)
- [ ] Understand: What are the two fields every Python object has?
- [ ] Find where `Py_INCREF` and `Py_DECREF` are defined
- [ ] Question: Why does CPython use reference counting instead of tracing GC?

### 1.2 PyVarObject - Variable-Length Objects
- [ ] Find `PyVarObject` in the headers
- [ ] Question: What's the difference between PyObject and PyVarObject?
- [ ] Which built-in types use PyVarObject? (hint: list, tuple, but not dict)
- [ ] **For Record:** Should Record use PyObject or PyVarObject? Why?

### 1.3 Type Objects
- [ ] Read `Include/cpython/object.h` - find `PyTypeObject` (around line 191)
- [ ] Identify the "slots" - tp_hash, tp_repr, tp_dealloc, tp_getattro, etc.
- [ ] Question: How does Python know what `len(x)` should call for a given type?
- [ ] Find where `PyTuple_Type` is defined in `Objects/tupleobject.c` - study its structure

---

## Phase 2: Concrete Data Structures

### 2.1 Tuples (Our Reference Implementation)
- [ ] Read `Include/cpython/tupleobject.h` and `Objects/tupleobject.c`
- [ ] Study the PyTupleObject struct - how does it store elements?
- [ ] Find `tuple_hash` - how does tuple compute its hash?
- [ ] Find `tuple_richcompare` - how does equality work?
- [ ] **For Record:** Our Record will store values like tuple, but add field names

### 2.2 Dictionaries (For Field Name Lookup)
- [ ] Read `Include/cpython/dictobject.h` - understand PyDictObject basics
- [ ] Question: How would we map field names to indices efficiently?
- [ ] Study `PyDict_GetItem` - how to look up a key

### 2.3 Designing Record's Memory Layout
- [ ] Sketch the RecordObject struct:
  - What fields do we need? (field names, values, cached hash?)
  - Should we store field names per-instance or share them?
- [ ] Question: What's the tradeoff between storing names as tuple vs dict?

---

## Phase 3: Type Slots Deep Dive

### 3.1 Essential Slots for Record
- [ ] `tp_dealloc` - Study tuple's dealloc. What must we DECREF?
- [ ] `tp_repr` - Study tuple's repr. How do we build the output string?
- [ ] `tp_hash` - Study tuple's hash. What makes a good hash for immutable containers?
- [ ] `tp_richcompare` - Study tuple's compare. Handle Py_EQ at minimum

### 3.2 Sequence Protocol (for indexing)
- [ ] Find `PySequenceMethods` in headers
- [ ] Study `sq_length` - returns `Py_ssize_t`
- [ ] Study `sq_item` - takes index, returns item (with INCREF!)
- [ ] **For Record:** Implement these so `r[0]` and `len(r)` work

### 3.3 Attribute Access (for field names)
- [ ] Study `tp_getattro` - how does attribute lookup work?
- [ ] Look at how namedtuple does it (it's in Python, but concept applies)
- [ ] **For Record:** Map `r.fieldname` to the correct value

### 3.4 Constructor
- [ ] Study `tp_new` vs `tp_init` - what's the difference?
- [ ] For immutable types, which one do we need?
- [ ] **For Record:** Design the C function signature for creating records

---

## Phase 4: The Evaluation Loop

### 4.1 ceval.c Overview
- [ ] Open `Python/ceval.c` - find `_PyEval_EvalFrameDefault` (around line 1577)
- [ ] Understand the main dispatch loop structure
- [ ] Find the stack macros: `PUSH()`, `POP()`, `TOP()`, `PEEK()` (around line 1391)

### 4.2 Study Similar Opcodes
- [ ] Find `BUILD_TUPLE` implementation - how does it pop N items and push a tuple?
- [ ] Find `BUILD_MAP` implementation - how does it handle key-value pairs?
- [ ] Question: What error handling pattern do these opcodes use?

### 4.3 Design BUILD_RECORD
- [ ] Decide on stack layout: `BUILD_RECORD n` where n is field count
- [ ] Stack before: `[..., name1, val1, name2, val2, ...]` (or different order?)
- [ ] Stack after: `[..., record]`
- [ ] What validation do we need? (names must be strings, no duplicates?)

---

## Phase 5: Implementation

### 5.1 Create the Header File
- [ ] Create `Include/recordobject.h`
- [ ] Define `RecordObject` struct
- [ ] Declare `PyRecord_Type`
- [ ] Declare `PyRecord_New()` constructor function

### 5.2 Implement the Type
- [ ] Create `Objects/recordobject.c`
- [ ] Implement `record_dealloc`
- [ ] Implement `record_repr`
- [ ] Implement `record_hash`
- [ ] Implement `record_richcompare`
- [ ] Implement `record_length` (sq_length)
- [ ] Implement `record_item` (sq_item)
- [ ] Implement `record_getattro` (attribute access by name)
- [ ] Define `PyRecord_Type` with all slots filled
- [ ] Implement `PyRecord_New()` - the C API constructor

### 5.3 Add the Opcode
- [ ] Add `BUILD_RECORD` to `Lib/opcode.py` (pick unused number, needs argument)
- [ ] Run `make regen-opcode` and `make regen-opcode-targets`
- [ ] Implement `BUILD_RECORD` handler in `Python/ceval.c`

### 5.4 Build System Integration
- [ ] Add `recordobject.c` to the build (Makefile.pre.in or setup.py)
- [ ] Add header to appropriate include lists
- [ ] Register type in Python initialization

### 5.5 Build and Test
- [ ] Run `make` - fix any compilation errors
- [ ] Test basic creation via C API
- [ ] Test via manual bytecode or compiler modification
- [ ] Verify all operations: indexing, len, hash, repr, equality, attribute access

---

## Verification Checklist

After implementation, verify each feature:

```python
# Creation (via whatever mechanism we build)
r = Record(x=10, y=20)

# Repr
assert repr(r) == "Record(x=10, y=20)"

# Indexing (sequence protocol)
assert r[0] == 10
assert r[1] == 20
assert len(r) == 2

# Attribute access
assert r.x == 10
assert r.y == 20

# Hashing (for use as dict key)
d = {r: "value"}
assert d[r] == "value"

# Equality
r2 = Record(x=10, y=20)
assert r == r2

# Immutability (should raise)
# r.x = 30  # AttributeError
# r[0] = 30  # TypeError
```

---

## Files We'll Create/Modify

| File | Action | ~Lines |
|------|--------|--------|
| `Include/recordobject.h` | Create | 25 |
| `Objects/recordobject.c` | Create | 200 |
| `Lib/opcode.py` | Modify | 2 |
| `Python/ceval.c` | Modify | 30 |
| `Makefile.pre.in` | Modify | 5 |
| `Python/bltinmodule.c` | Modify | 10 |

---

## How to Use This Guide

1. Read the specified files - don't skim, trace through the code
2. Answer questions before moving on (write answers down)
3. Use `./python.exe -c "..."` to experiment
4. Use GDB when confused: `gdb ./python.exe`
5. The `dis` module shows bytecode: `import dis; dis.dis(func)`

Debug build helpers:
```bash
./python.exe -c "import sys; print(sys.getrefcount(x))"
./python.exe -X showrefcount
```
