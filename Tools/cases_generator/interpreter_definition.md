# A higher level definition of the bytecode interpreter

## Abstract

The CPython interpreter is defined in C, meaning that the semantics of the
bytecode instructions, the dispatching mechanism, error handling, and
tracing and instrumentation are all intermixed.

This document proposes defining a custom C-like DSL for defining the 
instruction semantics and tools for generating the code deriving from
the instruction definitions.

These tools would be used to:
* Generate the main interpreter (done)
* Generate the tier 2 interpreter
* Generate documentation for instructions
* Generate metadata about instructions, such as stack use (done).

Having a single definition file ensures that there is a single source
of truth for bytecode semantics.

Other tools that operate on bytecodes, like `frame.setlineno`
and the `dis` module, will be derived from the common semantic
definition, reducing errors.

## Motivation

The bytecode interpreter of CPython has traditionally been defined as standard
C code, but with a lot of macros.
The presence of these macros and the nature of bytecode interpreters means
that the interpreter is effectively defined in a domain specific language (DSL).

Rather than using an ad-hoc DSL embedded in the C code for the interpreter,
a custom DSL should be defined and the semantics of the bytecode instructions,
and the instructions defined in that DSL.

Generating the interpreter decouples low-level details of dispatching
and error handling from the semantics of the instructions, resulting
in more maintainable code and a potentially faster interpreter.

It also provides the ability to create and check optimizers and optimization
passes from the semantic definition, reducing errors.

## Rationale

As we improve the performance of CPython, we need to optimize larger regions
of code, use more complex optimizations and, ultimately, translate to machine
code. 

All of these steps introduce the possibility of more bugs, and require more code
to be written. One way to mitigate this is through the use of code generators.
Code generators decouple the debugging of the code (the generator) from checking
the correctness (the DSL input).

For example, we are likely to want a new interpreter for the tier 2 optimizer
to be added in 3.12. That interpreter will have a different API, a different
set of instructions and potentially different dispatching mechanism.
But the instructions it will interpret will be built from the same building
blocks as the instructions for the tier 1 (PEP 659) interpreter.

Rewriting all the instructions is tedious and error-prone, and changing the
instructions is a maintenance headache as both versions need to be kept in sync.

By using a code generator and using a common source for the instructions, or 
parts of instructions, we can reduce the potential for errors considerably.


## Specification

This specification is a work in progress.
We update it as the need arises.

### Syntax

Each op definition has a kind, a name, a stack and instruction stream effect,
and a piece of C code describing its semantics::
 
```
  file:
    (definition | family | pseudo)+

  definition:
    "inst" "(" NAME ["," stack_effect] ")" "{" C-code "}"
    |
    "op" "(" NAME "," stack_effect ")" "{" C-code "}"
    |
    "macro" "(" NAME ")" "=" uop ("+" uop)* ";"
 
  stack_effect:
    "(" [inputs] "--" [outputs] ")"

  inputs:
    input ("," input)*

  outputs:
    output ("," output)*

  input:
    object | stream | array

  output:
    object | array

  uop:
    NAME | stream

  object:
    NAME [":" type] [ "if" "(" C-expression ")" ]

  type:
    NAME ["*"]

  stream:
    NAME "/" size

  size:
    INTEGER

  array:
    object "[" C-expression "]"

  family:
    "family" "(" NAME ")" = "{" NAME ("," NAME)+ [","] "}" ";"

  pseudo:
    "pseudo" "(" NAME ")" = "{" NAME ("," NAME)+ [","] "}" ";"
```

The following definitions may occur:

* `inst`: A normal instruction, as previously defined by `TARGET(NAME)` in `ceval.c`.
* `op`: A part instruction from which macros can be constructed.
* `macro`: A bytecode instruction constructed from ops and cache effects.

`NAME` can be any ASCII identifier that is a C identifier and not a C or Python keyword.
`foo_1` is legal. `$` is not legal, nor is `struct` or `class`.

The optional `type` in an `object` is the C type. It defaults to `PyObject *`.
The objects before the "--" are the objects on top of the stack at the start of
the instruction. Those after the "--" are the objects on top of the stack at the
end of the instruction.

An `inst` without `stack_effect` is a transitional form to allow the original C code
definitions to be copied. It lacks information to generate anything other than the
interpreter, but is useful for initial porting of code.

Stack effect names may be `unused`, indicating the space is to be reserved
but no use of it will be made in the instruction definition.
This is useful to ensure that all instructions in a family have the same
stack effect.

The number in a `stream` define how many codeunits are consumed from the
instruction stream. It returns a 16, 32 or 64 bit value.
If the name is `unused` the size can be any value and that many codeunits
will be skipped in the instruction stream.

By convention cache effects (`stream`) must precede the input effects.

The name `oparg` is pre-defined as a 32 bit value fetched from the instruction stream.

### Special functions/macros

The C code may include special functions that are understood by the tools as
part of the DSL.

Those functions include:

* `DEOPT_IF(cond, instruction)`. Deoptimize if `cond` is met.
* `ERROR_IF(cond, label)`. Jump to error handler at `label` if `cond` is true.
* `DECREF_INPUTS()`. Generate `Py_DECREF()` calls for the input stack effects.

Note that the use of `DECREF_INPUTS()` is optional -- manual calls
to `Py_DECREF()` or other approaches are also acceptable
(e.g. calling an API that "steals" a reference).

Variables can either be defined in the input, output, or in the C code.
Variables defined in the input may not be assigned in the C code.
If an `ERROR_IF` occurs, all values will be removed from the stack;
they must already be `DECREF`'ed by the code block.
If a `DEOPT_IF` occurs, no values will be removed from the stack or
the instruction stream; no values must have been `DECREF`'ed or created.

These requirements result in the following constraints on the use of
`DEOPT_IF` and `ERROR_IF` in any instruction's code block:

1. Until the last `DEOPT_IF`, no objects may be allocated, `INCREF`ed,
   or `DECREF`ed.
2. Before the first `ERROR_IF`, all input values must be `DECREF`ed,
   and no objects may be allocated or `INCREF`ed, with the exception
   of attempting to create an object and checking for success using
   `ERROR_IF(result == NULL, label)`. (TODO: Unclear what to do with
   intermediate results.)
3. No `DEOPT_IF` may follow an `ERROR_IF` in the same block.

(There is some wiggle room: these rules apply to dynamic code paths,
not to static occurrences in the source code.)

If code detects an error condition before the first `DECREF` of an input,
two idioms are valid:

- Use `goto error`.
- Use a block containing the appropriate `DECREF` calls ending in
  `ERROR_IF(true, error)`.

An example of the latter would be:
```cc
    res = PyObject_Add(left, right);
    if (res == NULL) {
        DECREF_INPUTS();
        ERROR_IF(true, error);
    }
```

### Semantics

The underlying execution model is a stack machine.
Operations pop values from the stack, and push values to the stack.
They also can look at, and consume, values from the instruction stream.

All members of a family
(which represents a specializable instruction and its specializations)
must have the same stack and instruction stream effect.

The same is true for all members of a pseudo instruction
(which is mapped by the bytecode compiler to one of its members).

## Examples

(Another source of examples can be found in the [tests](test_generator.py).)

Some examples:

### Output stack effect
```C
    inst ( LOAD_FAST, (-- value) ) {
        value = frame->f_localsplus[oparg];
        Py_INCREF(value);
    }
```
This would generate:
```C
    TARGET(LOAD_FAST) {
        PyObject *value;
        value = frame->f_localsplus[oparg];
        Py_INCREF(value);
        PUSH(value);
        DISPATCH();
    }
```

### Input stack effect
```C
    inst ( STORE_FAST, (value --) ) {
        SETLOCAL(oparg, value);
    }
```
This would generate:
```C
    TARGET(STORE_FAST) {
        PyObject *value = PEEK(1);
        SETLOCAL(oparg, value);
        STACK_SHRINK(1);
        DISPATCH();
    }
```

### Input stack effect and cache effect
```C
    op ( CHECK_OBJECT_TYPE, (owner, type_version/2 -- owner) ) {
        PyTypeObject *tp = Py_TYPE(owner);
        assert(type_version != 0);
        DEOPT_IF(tp->tp_version_tag != type_version);
    }
```
This might become (if it was an instruction):
```C
    TARGET(CHECK_OBJECT_TYPE) {
        PyObject *owner = PEEK(1);
        uint32 type_version = read32(next_instr);
        PyTypeObject *tp = Py_TYPE(owner);
        assert(type_version != 0);
        DEOPT_IF(tp->tp_version_tag != type_version);
        next_instr += 2;
        DISPATCH();
    }
```

### More examples

For explanations see "Generating the interpreter" below.)
```C
    op ( CHECK_HAS_INSTANCE_VALUES, (owner -- owner) ) {
        PyDictOrValues dorv = *_PyObject_DictOrValuesPointer(owner);
        DEOPT_IF(!_PyDictOrValues_IsValues(dorv));
    }
```
```C
    op ( LOAD_INSTANCE_VALUE, (owner, index/1 -- null if (oparg & 1), res) ) {
        res = _PyDictOrValues_GetValues(dorv)->values[index];
        DEOPT_IF(res == NULL);
        Py_INCREF(res);
        null = NULL;
        Py_DECREF(owner);
    }
```
```C
    macro ( LOAD_ATTR_INSTANCE_VALUE ) =
        counter/1 + CHECK_OBJECT_TYPE + CHECK_HAS_INSTANCE_VALUES +
        LOAD_INSTANCE_VALUE + unused/4 ;
```
```C
    op ( LOAD_SLOT, (owner, index/1 -- null if (oparg & 1), res) ) {
        char *addr = (char *)owner + index;
        res = *(PyObject **)addr;
        DEOPT_IF(res == NULL);
        Py_INCREF(res);
        null = NULL;
        Py_DECREF(owner);
    }
```
```C
    macro ( LOAD_ATTR_SLOT ) = counter/1 + CHECK_OBJECT_TYPE + LOAD_SLOT + unused/4;
```
```C
    inst ( BUILD_TUPLE, (items[oparg] -- tuple) ) {
        tuple = _PyTuple_FromArraySteal(items, oparg);
        ERROR_IF(tuple == NULL, error);
    }
```
```C
    inst ( PRINT_EXPR ) {
        PyObject *value = POP();
        PyObject *hook = _PySys_GetAttr(tstate, &_Py_ID(displayhook));
        PyObject *res;
        if (hook == NULL) {
            _PyErr_SetString(tstate, PyExc_RuntimeError,
                                "lost sys.displayhook");
            Py_DECREF(value);
            goto error;
        }
        res = PyObject_CallOneArg(hook, value);
        Py_DECREF(value);
        ERROR_IF(res == NULL);
        Py_DECREF(res);
    }
```

### Defining an instruction family

A _family_ maps a specializable instruction to its specializations.

Example: These opcodes all share the same instruction format):
```C
    family(load_attr) = { LOAD_ATTR, LOAD_ATTR_INSTANCE_VALUE, LOAD_SLOT };
```

### Defining a pseudo instruction

A _pseudo instruction_ is used by the bytecode compiler to represent a set of possible concrete instructions.

Example: `JUMP` may expand to `JUMP_FORWARD` or `JUMP_BACKWARD`:
```C
    pseudo(JUMP) = { JUMP_FORWARD, JUMP_BACKWARD };
```


## Generating the interpreter

The generated C code for a single instruction includes a preamble and dispatch at the end
which can be easily inserted. What is more complex is ensuring the correct stack effects
and not generating excess pops and pushes.

For example, in `CHECK_HAS_INSTANCE_VALUES`, `owner` occurs in the input, so it cannot be
redefined. Thus it doesn't need to written and can be read without adjusting the stack pointer.
The C code generated for `CHECK_HAS_INSTANCE_VALUES` would look something like:

```C
    {
        PyObject *owner = stack_pointer[-1];
        PyDictOrValues dorv = *_PyObject_DictOrValuesPointer(owner);
        DEOPT_IF(!_PyDictOrValues_IsValues(dorv));
    }
```

When combining ops together to form instructions, temporary values should be used,
rather than popping and pushing, such that `LOAD_ATTR_SLOT` would look something like:

```C
    case LOAD_ATTR_SLOT: {
        PyObject *s1 = stack_pointer[-1];
        /* CHECK_OBJECT_TYPE */
        {
            PyObject *owner = s1;
            uint32_t type_version = read32(next_instr + 1);
            PyTypeObject *tp = Py_TYPE(owner);
            assert(type_version != 0);
            if (tp->tp_version_tag != type_version) goto deopt;
        }
        /* LOAD_SLOT */
        {
            PyObject *owner = s1;
            uint16_t index = *(next_instr + 1 + 2);
            char *addr = (char *)owner + index;
            PyObject *null;
            PyObject *res = *(PyObject **)addr;
            if (res == NULL) goto deopt;
            Py_INCREF(res);
            null = NULL;
            Py_DECREF(owner);
            if (oparg & 1) {
                stack_pointer[0] = null;
                stack_pointer += 1;
            }
            s1 = res;
        }   
        next_instr += (1 + 1 + 2 + 1 + 4);
        stack_pointer[-1] = s1;
        DISPATCH();
    }
```

## Other tools

From the instruction definitions we can generate the stack marking code used in `frame.set_lineno()`,
and the tables for use by disassemblers.
