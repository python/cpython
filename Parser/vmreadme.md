Pegen Virtual Machine
=====================

The Pegen VM is an alternative for the recursive-descent Pegen parser.
The grammar (including actions) is identical, but execution does not
use the C stack.  We expect this to be faster, and initial
measurements seem to bear this out.  But we need to keep diligent, and
if it ever becomes clear that it will *not* be faster, we should stop
working on this project.

The runtime uses the same `Parser` structure as the recursive-descent
Pegen parser, and the same helper functions
(e.g., `_PyPegen_singleton_seq`).

The VM uses a stack to hold state during parsing.  The grammar is
represented by a few read-only tables.  The actions are represented by
a function containing a giant switch with one case per action.  (An
optimization here could be to combine identical actions.)

The grammar tables and the action function are meant to be generated
by a parser generator similar to the current one.  Because of the
actions, it needs to generate C code.

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

State related to loops is described below.

Note that `rule` doesn't change after the frame is initialized.  Also,
`mark` normally doesn't change, except for loop operations.

Each frame also has an array of values where successfully recognized
tokens and rules are stores.  This uses:

- `int ival`     -- number of values stored so far
- `void *vals[]` -- values stored (the type is `Token *` or an AST
  node type; may be NULL)

A `Rule` structure has the following fields:

- `char *name`    -- rule name, for debugging (e.g., `"start"`)
- `int type`      -- rule type, used for memo lookup
- `int alts[]`    -- index into `opcodes` array for each alternative,
                     terminated by `-1`
- `int opcodes[]` -- array of opcodes and their arguments

All rules are combined in a single array; the index in this array
is used by operations that reference other rules.

The `opcodes` array is a sequence of operation codes and arguments.
Some opcodes (e.g., `OP_TOKEN`) are followed by an argument; others
(e.g., `OP_NAME`) are not.  Both are representable as integers.


Operations
----------

Most operations can succeed or fail, and produce a vaue if they
succeed.

If an operation succeeds, the value is appended to the frame's values
array (`vals`), and the VM proceeds to the next opcode.

If an operation fails, the VM resets the input to the frame's mark,
and resets the value array.  It then proceeds to the next alternative
of the frame's rule, if there is one and the frame's `cut` flag is not
set.  If the frame's `cut` flag is set, or if its rule has no more
alternatives, the frame is popped off the frame stack and the VM
proceeds with failure there.

Some operations manipulate other frame fields.

Calls into the support runtime can produce *errors* -- when an error
is detected, the VM exits immediately, returning `NULL`.


### General operations

The following opcodes take no argument.

- `OP_NOOP` -- succeed without a value.  (Used for opcode padding.)

- `OP_NAME` -- call `_PyPegen_name_token()`; fail if it returns
  `NULL`, otherwise succeeds with the return value.

- `OP_NUMBER` -- call `_PyPegen_number_token()`; same as `OP_NAME`.

- `OP_STRING` -- call `_PyPegen_string_token()`; same as `OP_NAME`.

- `OP_CUT` -- set the frame's `cut` flag; succeed without a value.

- `OP_OPTIONAL` -- succeed without a value; modifies the *previous*
  operation to treat a `NULL` result as a success.  (See below.)

The following operations are followed by a single integer argument.

- `OP_TOKEN(type)` -- call `_PyPegen_expect_token()` with the `type`
  argument; processing is the same as for `OP_NAME`.

- `OP_RULE(rule)` -- push a new frame onto the stack, initializing it
  with the give rule (by index), the current input position (mark),
  at the first alternative and opcode.  Then proceed to the first
  operation of the new frame.

- `OP_RETURN(action)` -- call the action given by the argument, then
  pop the frame off the stack.  Execution then proceeds (in the frame
  newly revealed by that pop operation) as if the previous operation
  succeeded or failed with the return value of the action.


### Operations for root rules only

A grammar must have one or more *root rules*.  A root rule is a
synthetic rule that uses `OP_SUCCESS` and `OP_FAILURE` operations to
report overall success or failure.  Only root rules may use these
operations, and they must be used in the right format.

Each root rule must have exactly two alternatives.  The first
alternative must be a single operation (generally `OP_RULE`) that
stores a value in the values array, followed by `OP_SUCCESS`.  The
second alternative must be the single operation `OP_FAILURE`.

- `OP_SUCCESS` -- exit the VM with the first value from the `vals`
  array as the result.

- `OP_FAILURE` -- report a syntax error and exit the VM with a NULL
  result.


### Operations for loop rules only

For a loop such as `a*` or `a+`, a synthetic rule must be created with
the following structure:

```
# First alternative:
<one operation that produces a value, e.g. OP_NAME, OP_TOKEN, OP_RULE>
OP_LOOP_ITERATE

# Second alternative:
<either OP_LOOP_COLLECT or OP_LOOP_COLLECT_NONEMPTY>
```

The values being collected are stored in a `malloc`-ed array named
`collections` that is grown as needed.  This uses the following
fields:

- `ncollected` -- the number of collected values.
- `collection` -- `malloc`-ed array of `void *` values representing
  the collected values.

The operations are defined as follows:

- `OP_LOOP_ITERATE` -- append the current value to the `collections`
  array, save the current input position, and start the next iteration
  of the loop (resetting the instruction pointer).

- `OP_LOOP_COLLECT` -- restore the input position from the last saved
  position and pop the frame off the stack, producing a new value that
  is an `asdl_seq *` containing the collected values.

- `OP_LOOP_COLLECT_NONEMPTY` -- like `OP_LOOP_COLLECT` but fails if no
  values are collected.

For a "delimited" loop, written in the metagrammar as `b.a+` (one or
more `a` items separated by the delimiter `b`), the format is
slightly different:

```
# First alternative:
<one operation that produces the value for 'a' (the "meat")>
<one operation that produces the value for 'b' (the delimiter)>
OP_LOOP_ITERATE

# Second alternative:
<one operation that produces the value for 'a' (the "meat")>
OP_LOOP_COLLECT_DELIMITED
```

The new operation is:

- `OP_LOOP_COLLECT_DELIMITED` -- Add the first value from the values array
  to the collection and then do everything that `OP_LOOP_COLLECT does.


### Operations for lookaheads

Positive lookaheads use the following pattern:

```
OP_SAVE_MARK
<one operation that produces a value>
OP_POS_LOOKAHEAD
```

The operations work as follows:

- `OP_SAVE_MARK` -- saves the current input position in a dedicated
  field of the frame, `savemark`.

- `OP_POS_LOOKAHEAD` -- restores the current input position from the
  frame's `savemark` field.  (It does not reset the values array;
  values produced by positive lookaheads are ignored by the actions.

Negative lookaheads use the following pattern:

```
OP_SAVE_MARK
<one operation that produces a value>
OP_NEG_LOOKAHEAD
```

The new operation works as follows:

- `OP_NEG_LOOKAHEAD` -- fails the current alternative.

In addition, the standard code for success/failure processing checks
whether the next operation is `OP_NEG_LOOKAHEAD`.  If so, it treats
`NULL` as a success and restores the current input position from the
frame's `savemark` field.


More about `OP_OPTIONAL`
------------------------

The `OP_OPTIONAL` flag is a "postfix" operation.  It must *follow* any
operation that may produce a result.  The normal way of the VM is that
if the result is `NULL` this is treated as a failure by the VM.  But
before treating `NULL` as a failure, the VM checks whether the next
operation is `OP_OPTIONAL`.  If so, it treats `NULL` as a success.  In
this case a `NULL` is appended to the `vals` array and control flows
to the next operation.

When the operation preceding `OP_OPTIONAL` succeeds, `OP_OPTIONAL` is
executed as a regular operation, and always succeed.

The `OP_NEG_LOOKAHEAD` works similar (but it also restores the input
position).


Constraints on operation order
------------------------------

Note that for operations that succeed with a value or fail, there is
always a next operation.  These operations are `OP_NAME`, `OP_NUMBER`,
`OP_STRING`, `OP_TOKEN`, and `OP_RULE`.

These operations always succeed: `OP_NOOP`, `OP_CUT`, `OP_OPTIONAL`,
`OP_START`.

These operations must be last in their alternative: `OP_RETURN`,
`OP_SUCCESS`, `OP_FAILURE`, `OP_LOOP_ITERATE`, `OP_LOOP_COLLECT`,
`OP_LOOP_COLLECT_NONEMPTY`.

This operation must be first in its alternative: `OP_FAILURE`.


Grammar for lists of operations
-------------------------------

This shows the constraints on how operations can be used together.

```
rule: root_rule | normal_rule | loop_rule | delimited_rule

root_rule: success_alt failure_alt

success_alt: regular_op OP_SUCCESS
failure_alt: OP_FAILURE

normal_rule: alt+

loop_rule: loop_start_alt loop_collect_alt

loop_start_alt: regular_op OP_LOOP_ITERATE
loop_collect_alt: OP_LOOP_COLLECT | OP_LOOP_COLLECT_NONEMPTY

delimited_rule: delimited_start_alt delimited_collect_alt

delimited_start_alt: regular_op regular_op OP_LOOP_ITERATE
delimited_collect_alt: OP_LOOP_COLLECT_DELIMITED

alt: any_op+ return_op

any_op: regular_op [OP_OPTIONAL] | lookahead_block | special_op
regular_op: short_op | long_op
short_op: OP_NAME | OP_NUMBER | OP_STRING
long_op: OP_TOKEN <token_type> | OP_RULE <rule_id>
special_op: OP_NOOP | OP_CUT
return_op: OP_RETURN <action_id>

lookahead_block: OP_SAVE_MARK regular_op lookahead_op
lookahead_op:  OP_POS_LOOKAHEAD | OP_NEG_LOOKAHEAD
```

Ideas
-----

### Left-recursion

- First opcode of first alt is `OP_SETUP_LEFT_REC`.  This initializes
  the memo cache for f->rule->type to `NULL`.

- All `OP_RETURN` opcodes in a left-rec rule are replaced with
  `OP_RETURN_LEFT_REC`.  This compares the current position with the
  most recently cached position for this rule at this point in the
  input.  If the new match is *further*, it updates the memo cache,
  resets `f->mark`, and resets `f->iop` and `f->ialt` to the start of
  the rule.  It then goes back to the top (possibly skipping the setup
  op).  If the new match is not further, it is discarded and the most
  recent match from the memo cache is returned as the result (also
  updating the end position).

### Selective memoization

- We could have a flag in the rule that prevents memo lookups and
  inserts.  Or we could have separate opcodes, e.g. `OP_RULE_NOMEMO`
  and `OP_RETURN_NOMEMO`.

### Performance tuning

- To make frames smaller, we could have a separate values stack; the
  frame would have a `void** vals` instead of `void *vals[]`.  Most
  frames won't need 20 values, but we could ensure there are always at
  least that many on the stack.

- Is it faster to check for flags in the rule object (e.g. leftrec) or
  is it faster to have dedicated opcodes?  My current hunch is that
  dedicated opcodes are faster, but I really don't know.  Maybe having
  fewer opcodes is more important than having smaller Rule objects.

- Random other C tricks, esp. tricks that might increase the hit rate
  in the CPU's L1 cache.  (Remember in modern CPUs memory access is
  10-100x slower than cache access.)  Who can we tap that knows this
  stuff?

- If we know `x >= -1`, Which is faster: `if (x < 0)` or `if (x == -1)`?
