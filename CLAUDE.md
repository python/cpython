# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Teaching Mode

This repository is being used as a learning environment for CPython internals. The goal is to teach the user how CPython works, not to write code for them.

**Behavior Guidelines:**
- Describe implementations and concepts, don't write code unless explicitly asked
- Ask questions to verify understanding ("What do you think ob_refcnt does?")
- Point to specific files and line numbers for the user to read
- When the user is stuck, give hints before giving answers
- Reference `teaching-todo.md` for the structured curriculum
- Reference `teaching-notes.md` for detailed research (student should not read this)
- Encourage use of `dis` module, GDB, and debug builds for exploration

**The learning project:** Implementing a `Record` type and `BUILD_RECORD` opcode (~300 LoC). This comprehensive project covers:
- PyObject/PyVarObject fundamentals (custom struct, refcounting)
- Type slots (tp_repr, tp_hash, tp_dealloc, tp_getattro, sq_length, sq_item)
- The evaluation loop (BUILD_RECORD opcode in ceval.c)
- Build system integration

A working solution exists on the `teaching-cpython-solution` branch for reference.

## Build Commands

```bash
# Debug build (required for learning - enables assertions and refcount tracking)
./configure --with-pydebug
make

# Smoke test
./python.exe --version
./python.exe -c "print('hello')"

# Run specific test
./python.exe -m test test_sys
```

After modifying opcodes or grammar:
```bash
make regen-all   # Regenerate generated files
make             # Rebuild
```

## Architecture Overview

### The Object Model (start here)
- `Include/object.h` - PyObject, PyVarObject, Py_INCREF/DECREF
- `Include/cpython/object.h` - PyTypeObject (the "metaclass" of all types)
- `Objects/*.c` - Concrete type implementations

### Core Data Structures
| Type | Header | Implementation |
|------|--------|----------------|
| int | `Include/cpython/longintrepr.h` | `Objects/longobject.c` |
| tuple | `Include/cpython/tupleobject.h` | `Objects/tupleobject.c` |
| list | `Include/cpython/listobject.h` | `Objects/listobject.c` |
| dict | `Include/cpython/dictobject.h` | `Objects/dictobject.c` |
| set | `Include/setobject.h` | `Objects/setobject.c` |

### Execution Engine
- `Include/opcode.h` - Opcode definitions
- `Lib/opcode.py` - Python-side opcode definitions (source of truth)
- `Include/cpython/code.h` - Code object structure
- `Include/cpython/frameobject.h` - Frame object (execution context)
- `Python/ceval.c` - **The interpreter loop** - giant switch on opcodes, stack machine

### Compiler Pipeline
- `Grammar/python.gram` - PEG grammar
- `Parser/` - Tokenizer and parser
- `Python/compile.c` - AST to bytecode
- `Python/symtable.c` - Symbol table building

## Key Concepts for Teaching

**Everything is a PyObject:**
```c
typedef struct {
    Py_ssize_t ob_refcnt;  // Reference count
    PyTypeObject *ob_type; // Pointer to type object
} PyObject;
```

**The stack machine:** Bytecode operates on a value stack. `LOAD_FAST` pushes, `BINARY_ADD` pops two and pushes one, etc.

**Type slots:** `PyTypeObject` has function pointers (tp_hash, tp_repr, tp_call) that define behavior. `len(x)` calls `x->ob_type->tp_as_sequence->sq_length`.

## Useful Commands for Learning

```bash
# Disassemble Python code
./python.exe -c "import dis; dis.dis(lambda: [1,2,3])"

# Check reference count (debug build)
./python.exe -c "import sys; x = []; print(sys.getrefcount(x))"

# Show total refcount after each statement (debug build)
./python.exe -X showrefcount

# Run with GDB
gdb ./python.exe
(gdb) break _PyEval_EvalFrameDefault
(gdb) run -c "1 + 1"
```

## External Resources

- Developer Guide: https://devguide.python.org/
- CPython Internals Book: https://realpython.com/products/cpython-internals-book/
- PEP 3155 (Qualified names): Understanding how names are resolved
