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
- [x] Read `Include/object.h` - find `PyObject` struct (around line 105)
- [x] Understand: What are the two fields every Python object has?
- [x] Find where `Py_INCREF` and `Py_DECREF` are defined
- [x] Question: Why does CPython use reference counting instead of tracing GC?

**Notes from session:**
- Two fields: `ob_refcnt` (reference count) and `ob_type` (pointer to type object)
- `Py_INCREF`: increments `ob_refcnt`
- `Py_DECREF`: decrements `ob_refcnt`, calls `tp_dealloc` when it hits 0
- Debug builds also track `_Py_RefTotal` (global count of all refs) for leak detection
- Why refcounting?
  - Deterministic destruction (RAII-like) - resources freed immediately when last ref drops
  - C extensions can easily participate (just INCREF/DECREF, no root registration)
  - No GC pauses
  - Tradeoff: can't handle cycles alone, so CPython has supplemental cyclic GC
- C++ `shared_ptr` has same cycle limitation - that's why `weak_ptr` exists

### 1.2 PyVarObject - Variable-Length Objects
- [x] Find `PyVarObject` in the headers
- [x] Question: What's the difference between PyObject and PyVarObject?
- [x] Which built-in types use PyVarObject? (hint: list, tuple, but not dict)
- [x] **For Record:** Should Record use PyObject or PyVarObject? Why?

**Notes from session:**
- `PyVarObject` adds `ob_size` field - count of elements (not bytes!)
- Used with "flexible array member" pattern: struct ends with `item[1]`, allocate extra space
- `PyObject_HEAD` macro → embeds `PyObject ob_base`
- `PyObject_VAR_HEAD` macro → embeds `PyVarObject ob_base` (includes size)
- Macros are historical - used to expand to raw fields, now just embed the struct
- **Record decision:** Use `PyVarObject` because we store variable number of values

### 1.3 Type Objects
- [x] Read `Include/cpython/object.h` - find `PyTypeObject` (around line 191)
- [x] Identify the "slots" - tp_hash, tp_repr, tp_dealloc, tp_getattro, etc.
- [x] Question: How does Python know what `len(x)` should call for a given type?
- [x] Find where `PyTuple_Type` is defined in `Objects/tupleobject.c` - study its structure

**Notes from session:**
- `PyTypeObject` is a big struct of function pointers ("slots") that define type behavior
- Naming conventions: `tp_` (type), `ob_` (object), `sq_` (sequence), `nb_` (number), `mp_` (mapping)
- `len(x)` works by: `obj->ob_type->tp_as_sequence->sq_length` - if any pointer is NULL, raises TypeError
- This slot-based dispatch is what enables Python's duck typing - interpreter just checks "do you have this slot?"
- Slots are fast (2-3 pointer chases) vs method lookup (hashing, dict probe, MRO traversal)
- Adding new slots to PyTypeObject is an ABI-breaking change - done rarely and carefully
- `tp_vectorcall` added in 3.8, `tp_as_async` added in 3.5 (as sub-struct pointer to minimize slots added)
- Most slots established by Python 2.6 - ABI stability concerns limit additions
- New features often work around slots: decorators use function calls, context managers use method lookup

---

## Phase 2: Concrete Data Structures

### 2.1 Tuples (Our Reference Implementation)
- [x] Read `Include/cpython/tupleobject.h` and `Objects/tupleobject.c`
- [x] Study the PyTupleObject struct - how does it store elements?
- [x] Find `tuple_hash` - how does tuple compute its hash?
- [x] Find `tuple_richcompare` - how does equality work?
- [x] **For Record:** Our Record will store values like tuple, but add field names

**Notes from session:**
- `PyTupleObject` uses flexible array member: `ob_item[1]` with extra allocation
- `tuple_as_sequence` struct at ~line 804 holds function pointers for sequence ops
- Slots use 0 instead of NULL - same thing in C, just style preference (shorter in struct initializers)
- `tuplelength`: just calls `Py_SIZE(a)` to return ob_size
- `tupleitem`: **must INCREF before returning** - caller gets a "new reference" they must DECREF
  - If you forget INCREF: use-after-free bug when something else DECREFs the object
- `sq_ass_item` is 0 (NULL) because tuple is immutable - no assignment allowed
- **tuple_hash**: uses xxHash algorithm (XXPRIME constants), combines hashes of all elements
  - If any element is unhashable, returns -1 and propagates error
  - `-1` is reserved for error - actual -1 hash gets changed to -2 (a CPython wart)
- **tuple_richcompare**: `op` parameter is Py_EQ, Py_NE, Py_LT, etc.
  - Compares element by element until difference found
  - For Record: compare both names AND values; different field names = not equal

### 2.2 Dictionaries (For Field Name Lookup)
- [ ] Read `Include/cpython/dictobject.h` - understand PyDictObject basics
- [ ] Question: How would we map field names to indices efficiently?
- [ ] Study `PyDict_GetItem` - how to look up a key

### 2.3 Designing Record's Memory Layout
- [x] Sketch the RecordObject struct:
  - What fields do we need? (field names, values, cached hash?)
  - Should we store field names per-instance or share them?
- [x] Question: What's the tradeoff between storing names as tuple vs dict?

**Notes from session:**
- Designed struct:
  ```c
  typedef struct {
      PyObject_VAR_HEAD        // refcnt, type, ob_size (field count)
      PyObject *names;         // tuple of field name strings
      PyObject *values[1];     // flexible array of values
  } RecordObject;
  ```
- **Design decision:** Store names as separate tuple (not inline per-value)
  - Pros: Memory efficient when many records share same field names
  - Cons: Extra pointer per record, slightly more complex
  - Even with inline names, Python interns short strings, so same pointer - but still N extra pointers vs 1
- **Dealloc must DECREF:**
  - `names` (the tuple)
  - Each `values[i]` (the stored objects)
  - NOT the VAR_HEAD fields (ob_refcnt is ours, ob_type is static, ob_size is just int)

---

## Phase 3: Type Slots Deep Dive

### 3.1 Essential Slots for Record
- [x] `tp_dealloc` - Study tuple's dealloc. What must we DECREF?
- [x] `tp_repr` - Study tuple's repr. How do we build the output string?
- [x] `tp_hash` - Study tuple's hash. What makes a good hash for immutable containers?
- [x] `tp_richcompare` - Study tuple's compare. Handle Py_EQ at minimum

**Notes from session:**
- **tp_repr**: Uses `PyUnicodeWriter` to build strings piece by piece
  - `PyUnicodeWriter_WriteUTF8()` for literal strings, `PyUnicodeWriter_WriteStr()` for PyObjects
  - Call `PyObject_Repr()` on each value (returns new ref - must DECREF!)
  - `Py_ReprEnter`/`Py_ReprLeave` for cycle detection (prevents infinite recursion on self-referential structures)
- **tp_hash**: xxHash algorithm, combine element hashes
  - Return -1 for error (propagate from unhashable elements)
  - If actual hash is -1, change to -2 (CPython convention)
- **tp_richcompare**: For Record, compare names tuple AND values; return Py_NotImplemented for ordering ops (< > <= >=)

### 3.2 Sequence Protocol (for indexing)
- [x] Find `PySequenceMethods` in headers
- [x] Study `sq_length` - returns `Py_ssize_t`
- [x] Study `sq_item` - takes index, returns item (with INCREF!)
- [x] **For Record:** Implement these so `r[0]` and `len(r)` work

**Notes from session:**
- `PySequenceMethods` contains: `sq_length`, `sq_concat`, `sq_repeat`, `sq_item`, `sq_ass_item`, `sq_contains`, etc.
- For Record, we need:
  ```c
  static PySequenceMethods record_as_sequence = {
      (lenfunc)record_length,    /* sq_length */
      0,                         /* sq_concat */
      0,                         /* sq_repeat */
      (ssizeargfunc)record_item, /* sq_item */
      // rest NULL - Record is immutable
  };
  ```
- Pattern for `record_item`: bounds check, INCREF, return

### 3.3 Attribute Access (for field names)
- [x] Study `tp_getattro` - how does attribute lookup work?
- [ ] Look at how namedtuple does it (it's in Python, but concept applies)
- [x] **For Record:** Map `r.fieldname` to the correct value

**Notes from session:**
- `tp_getattr` (old, char*) vs `tp_getattro` (new, PyObject*) - use getattro
- Implementation: loop through `self->names`, compare with `name` using `PyUnicode_Compare`
- If found, INCREF and return `self->values[i]`
- If not found, fall back to `PyObject_GenericGetAttr()` for `__class__`, `__doc__`, etc.
- O(n) search is fine for small field counts; could use dict for optimization but not needed

### 3.4 Constructor
- [x] Study `tp_new` vs `tp_init` - what's the difference?
- [x] For immutable types, which one do we need?
- [x] **For Record:** Design the C function signature for creating records

**Notes from session:**
- `tp_new`: allocates AND initializes, returns new object
- `tp_init`: mutates already-created object
- For immutable types, use `tp_new` only (no mutation after creation)
- **Two constructors needed:**
  - `PyRecord_New(names, values, n)` - C API for BUILD_RECORD opcode, steals references
  - `record_new(type, args, kwargs)` - Python API (tp_new slot) for `Record(x=1, y=2)`, parses kwargs
- C API: `PyTuple_New` allocates, `PyTuple_Pack` is convenience wrapper
- For Record, only need `PyRecord_New` (one caller), `record_new` wraps it for Python

---

## Phase 4: The Evaluation Loop

### 4.1 ceval.c Overview
- [x] Open `Python/ceval.c` - find `_PyEval_EvalFrameDefault` (around line 1577)
- [x] Understand the main dispatch loop structure
- [x] Find the stack macros: `PUSH()`, `POP()`, `TOP()`, `PEEK()` (around line 1391)

### 4.2 Study Similar Opcodes
- [x] Find `BUILD_TUPLE` implementation - how does it pop N items and push a tuple?
- [x] Find `BUILD_MAP` implementation - how does it handle key-value pairs?
- [x] Question: What error handling pattern do these opcodes use?

**Notes from session:**
- Main loop is giant switch on opcodes in `_PyEval_EvalFrameDefault`
- `oparg` = argument encoded in bytecode (e.g., field count)
- Stack macros: `POP()`, `PUSH()`, `PEEK(n)`, `STACK_SHRINK(n)`
- **BUILD_TUPLE**: POPs n items, uses `PyTuple_SET_ITEM` which **steals** references (no DECREF needed)
- **BUILD_MAP**: Uses PEEK to read items, `PyDict_SetItem` which INCREFs, then POPs and DECREFs separately
- Error pattern: `if (result == NULL) goto error;`
- `DISPATCH()` jumps to next opcode

### 4.3 Design BUILD_RECORD
- [x] Decide on stack layout: `BUILD_RECORD n` where n is field count
- [x] Stack before: `[..., name1, val1, name2, val2, ...]` (or different order?)
- [x] Stack after: `[..., record]`
- [ ] What validation do we need? (names must be strings, no duplicates?)

**Notes from session:**
- **Stack layout decision:** `[..., names_tuple, val0, val1, val2]` (Option B)
  - names_tuple is pre-built (by BUILD_TUPLE or LOAD_CONST)
  - Simpler opcode, names can be shared across records
- `PyRecord_New` will steal references (like BUILD_TUPLE, not BUILD_MAP)
- Implementation sketch:
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

---

## Phase 5: Implementation

### 5.1 Create the Header File
- [x] Create `Include/recordobject.h`
- [x] Define `RecordObject` struct
- [x] Declare `PyRecord_Type`
- [x] Declare `PyRecord_New()` constructor function

**Notes from session:**
- Two-level header structure: `Include/recordobject.h` (public) includes `Include/cpython/recordobject.h` (internal)
- Must add `#include "recordobject.h"` to `Include/Python.h` for it to be visible
- `PyRecord_Check` macro simplified to `Py_IS_TYPE(op, &PyRecord_Type)` (no subclass flag)

### 5.2 Implement the Type
- [x] Create `Objects/recordobject.c`
- [x] Implement `record_dealloc`
- [x] Implement `record_repr`
- [x] Implement `record_hash`
- [x] Implement `record_richcompare`
- [x] Implement `record_length` (sq_length)
- [x] Implement `record_item` (sq_item)
- [x] Implement `record_getattro` (attribute access by name)
- [x] Define `PyRecord_Type` with all slots filled
- [x] Implement `PyRecord_New()` - the C API constructor

**Notes from session:**
- Started by copying tupleobject.c, then stripped out complexity (clinic, free lists, etc.)
- **GC gotcha:** Originally had GC alloc/track/untrack but no `Py_TPFLAGS_HAVE_GC` or `tp_traverse` - caused segfault. Fixed by removing GC entirely (simpler for learning)
- **record_dealloc:** `Py_XDECREF(names)` + XDECREF each value, use XDECREF (handles NULL safely)
- **record_repr:** Use `_PyUnicodeWriter`, format as `record(x=1, y=2)`, don't repr the field names (just write them directly)
- **record_hash:** Copied xxHash pattern from tuple, hash names tuple + all values, `-1` → `-2` for actual -1 hash
- **record_getattro:** Loop through names with `PyUnicode_Compare`, return value with INCREF, fallback to `PyObject_GenericGetAttr`
- **record_richcompare:** Compare names first (if different, not equal), then values; return `Py_NotImplemented` for ordering
- **record_new (tp_new):** Parse kwargs with `PyDict_Keys`/`PyDict_Values`, convert keys to tuple for names

### 5.3 Add the Opcode
- [x] Add `BUILD_RECORD` to `Lib/opcode.py` (pick unused number, needs argument)
- [x] Run `make regen-opcode` and `make regen-opcode-targets`
- [x] Implement `BUILD_RECORD` handler in `Python/ceval.c`

**Notes from session:**
- Used opcode 166 (was available)
- Stack layout: `[names_tuple, val0, val1, ...]` - pop values in reverse, then pop names
- Must add stack effect to compile.c: `case BUILD_RECORD: return -oparg;`
- Regenerate with `./python.exe ./Tools/scripts/generate_opcode_h.py` and `./python.exe ./Python/makeopcodetargets.py`

### 5.4 Build System Integration
- [x] Add `recordobject.c` to the build (Makefile.pre.in or setup.py)
- [x] Add header to appropriate include lists
- [x] Register type in Python initialization

**Notes from session:**
- Add `Objects/recordobject.o` to `OBJECT_OBJS` in Makefile.pre.in
- Add both headers to `PYTHON_HEADERS` section
- Register with `SETBUILTIN("record", &PyRecord_Type)` in `Python/bltinmodule.c`

### 5.5 Build and Test
- [x] Run `make` - fix any compilation errors
- [x] Test basic creation via C API
- [x] Test via manual bytecode or compiler modification
- [x] Verify all operations: indexing, len, hash, repr, equality, attribute access

**Notes from session:**
- All tests pass:
  ```python
  r = record(x=1, y=2)
  print(r)       # record(x=1, y=2)
  print(r.x)     # 1
  print(r[0])    # 1
  print(len(r))  # 2
  print(hash(r)) # works
  r2 = record(x=1, y=2)
  print(r == r2) # True
  ```

---

## Phase 6: Literal Syntax (Bonus)

### 6.1 Grammar Extension
- [x] Add `record` rule to `Grammar/python.gram`
- [x] Choose syntax: `{| x=1, y=2 |}`
- [x] Add to `atom` rule with lookahead `&'{'`

**Notes from session:**
- Reused `kwargs` from function call parsing for `x=1, y=2` syntax
- Had to use separate tokens `'{' '|'` not `'{|'` (multi-char tokens don't exist)
- Grammar rule: `| '{' '|' a=[kwargs] '|' '}' { _PyAST_Record(...) }`

### 6.2 AST Node
- [x] Add `Record(expr* keys, expr* values)` to `Parser/Python.asdl`
- [x] Run `make regen-ast` to generate `_PyAST_Record`

**Notes from session:**
- Modeled after `Dict(expr* keys, expr* values)`
- Regeneration creates `_PyAST_Record` function and `Record_kind` enum

### 6.3 Parser Helpers
- [x] Create `_PyPegen_get_record_keys` in `Parser/pegen.c`
- [x] Create `_PyPegen_get_record_values` in `Parser/pegen.c`

**Notes from session:**
- `kwargs` returns `KeywordOrStarred*` items, not `KeyValuePair*` like dict
- Had to unwrap each to get `keyword_ty`, then extract `.arg` (identifier) and `.value` (expression)
- For keys, create `Name` expression from the identifier

### 6.4 Compiler Integration
- [x] Add `compiler_record` function in `Python/compile.c`
- [x] Add `case Record_kind:` to `compiler_visit_expr`
- [x] Add `case Record_kind:` to ALL switches that handle `Dict_kind`:
  - `Python/ast.c` (2 switches!)
  - `Python/ast_opt.c`
  - `Python/ast_unparse.c`
  - `Python/symtable.c`
  - `Python/compile.c` (3 switches!)

**Notes from session:**
- **Key lesson:** Every switch on expression kinds needs the new case - easy to miss some!
- `compiler_record`: Build names tuple as constant, VISIT each value, emit BUILD_RECORD
- Extracting keys: `e->v.Record.keys` contains `expr_ty` Name nodes, access with `key_expr->v.Name.id`

### 6.5 Final Result
- [x] Literal syntax `{| x=1, y=2 |}` works
- [x] Emits BUILD_RECORD opcode
- [x] All features work with literal syntax

**Notes from session:**
- Full pipeline working: Grammar → Parser → AST → Compiler → Bytecode → Execution
- Test: `r = {|x=1, y=2|}; print(r.x)` → `1`

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

## Files Created/Modified

| File | Action | Notes |
|------|--------|-------|
| `Include/recordobject.h` | Create | Public API header |
| `Include/cpython/recordobject.h` | Create | Internal struct definition |
| `Include/Python.h` | Modify | Add include for recordobject.h |
| `Objects/recordobject.c` | Create | ~300 lines - type implementation |
| `Lib/opcode.py` | Modify | Add BUILD_RECORD = 166 |
| `Python/ceval.c` | Modify | BUILD_RECORD handler |
| `Python/compile.c` | Modify | compiler_record, stack effect, expr switches |
| `Makefile.pre.in` | Modify | Add recordobject.o and headers |
| `Python/bltinmodule.c` | Modify | SETBUILTIN for record |
| `Grammar/python.gram` | Modify | record rule, atom lookahead |
| `Parser/Python.asdl` | Modify | Record AST node |
| `Parser/pegen.c` | Modify | Helper functions for record keys/values |
| `Python/ast.c` | Modify | Record_kind in validation switches |
| `Python/ast_opt.c` | Modify | Record_kind case |
| `Python/ast_unparse.c` | Modify | append_ast_record |
| `Python/symtable.c` | Modify | Record_kind case |

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
