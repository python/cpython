Pegen Virtual Machine
=====================

The Pegen VM is an alternative for the recursive-descent Pegen parser.
The idea is that the grammar (including actions) is identical, but
execution does not use the C stack frame.  The hope is that this might
be faster, but this is far from given.  If it is clear that it will
*not* be faster, we should stop working on this project.

The runtime uses the same `Parser` structure as the recursive-descent
Pegen parser, and the same helper functions
(e.g., `_PyPegen_singleton_seq`).

The VM uses a stack to hold state during parsing.  The grammar is
represented by a few read-only tables.  This design may be familiar
from that of CPython's "ceval" VM.  The actions are represented by a
function containing a giant switch with one case for each action.

The grammar tables and the action function are meant to be generated
by a parser generator similar to the current one.  Because of the
actions, the generator needs to generate C code.

The primary VM state is a stack of `Frame` structures.  Each frame
represents a particular attempt to parse a rule at a given point in
the input.  The only state separate from the stack is a pointer to the
`Parser` structure.

The main state in a frame is as follows:

- `Rule *rule`   -- points to the rule being parsed
- `int mark`     -- the input position at the start of the rule invocation
- `int ialt`     -- indicates which alternative is currently being tried
- `int iop`      -- indicates where we are in the current alternative
- `int cut`      -- whether a "cut" was executed in the current alternative

(We'll need state related to line numbers and column offsets as well.)

Note that `rule` and `mark` don't change after the frame is initialized.

In addition each frame has a "value stack" where successfully
recognized tokens and rules are stored.  This uses:

- `int ival`     -- number of values stored so far
- `void *vals[]` -- values stored (the type is `Token *` or an AST node type)

A `Rule` structure has the following fields:

- `char *name`    -- rule name, for debugging (e.g., `"start"`)
- `int alts[]`    -- index into `opcodes` array for each alternative,
                     terminated by `-1`
- `int opcodes[]` -- array of opcodes and their arguments

All rules are collected in a single array; the index in this array
is used by operations that reference other rules.

The `opcodes` array is a sequence of operation codes and arguments.
Some opcodes (e.g., `OP_TOKEN`) are followed by an argument; others
(e.g., `OP_NAME`) are not.  Both are representable as integers.

Opcodes
-------

Most operations can succeed or fail, and produce a vaue if they
succeed.

If an operation succeeds, the value is appended to the frame's values
stack, and the VM proceeds to the next opcode.

If an operation fails, the VM resets the input to the frame's mark,
and then proceeds to the next alternative of the frame's rule, if
there is one, and the frame's `cut` flag is not set.  If the frame's
`cut` flag is set, or if its rule has no more alternatives, the frame
is popped off the frame stack and the VM proceeds with failure there.

Some operations manipulate other frame fields.

Calls into the support runtime can also produce *errors* -- when an
error is detected, the VM exits, immediately returning `NULL`.

The following opcodes take no argument.

- `OP_NAME` -- call `_PyPegen_name_token()`; fail if it returns
  `NULL`, otherwise succeeds with the return value.

- `OP_NUMBER` -- call `_PyPegen_number_token()`; same as `OP_NAME`.

- `OP_STRING` -- call `_PyPegen_string_token()`; same as `OP_NAME`.

- `OP_CUT` -- set the frame's `cut` flag; always succeeds, without a
  value.

- `OP_OPTIONAL` -- modifies the *previous* operation to treat a `NULL`
  result as a success.

- `OP_FAILURE` -- report a syntax error and exit the VM.

These opcodes are followed by a single integer argument.

- `OP_TOKEN` -- call `_PyPegen_expect_token()` with the argument;
  processing is the same as for `OP_NAME`.

- `OP_RULE` -- push a new frame onto the stack, initializing it with
  the given rule, the current input position (mark), at the first
  alternative and opcode.  Then proceed to the first operation of the
  new frame.

- `OP_RETURN` -- call the action given by the argument, then pop the
  frame off the stack.  Execution then proceeds (in the frame newly
  revealed by that pop operation) as if the previous operation
  succeeded or failed with the return value of the action.

- `OP_SUCCESS` -- call the action given by the argument, and exit the
  VM with its return value as result.

Ideas
-----

We need to extend the VM to support loops and lookaheads.

- Standard loops (`a*`, `a+`, `a.b+`) -- add a new opcode that resets
  the instruction pointer without destroying the captured value; need
  a new frame field to store the collected values.  (Probably needs
  several opcodes.)

- Positive lookahead (`&a`) -- add a new opcode to save the mark in a
  new frame field, and another opcode that restores the mark from
  there.

- Negative lookahead (`!a`) -- add another opcode that catches failure
  and turns it into success, restoring the saved mark.

- Left-recursion -- hmm, tricky...
