# Python VM State

## Definition of Tiers

- **Tier 1** is the classic Python bytecode interpreter.
  This includes the specializing [adaptive interpreter](../InternalDocs/adaptive.md).
- **Tier 2**, also known as the micro-instruction ("uop") interpreter, is a new execution engine.
  It was introduced in Python 3.13, and also forms the basis for a JIT using copy-and-patch technology. See [Tier 2](tier2_engine.md) for more information.



# Thread state and interpreter state

An important piece of VM state is the **thread state**, held in `tstate`.
The current frame pointer, `frame`, is always equal to `tstate->current_frame`.
The thread state also holds the exception state (`tstate->exc_info`) and the recursion counters (`tstate->c_recursion_remaining` and `tstate->py_recursion_remaining`).

The thread state is also used to access the **interpreter state** (`tstate->interp`), which is important since the "eval breaker" flags are stored there (`tstate->interp->ceval.eval_breaker`, an "atomic" variable), as well as the "PEP 523 function" (`tstate->interp->eval_frame`).
The interpreter state also holds the optimizer state (`optimizer` and some counters).
Note that the eval breaker may be moved to the thread state soon as part of the multicore (PEP 703) work.


## Tier 2

- `stack_pointer` is the same as in Tier 1 (but may be different in the JIT).
- At runtime we do not need a cache representation of `frame->instr_ptr`, as all stores to `frame->instr_ptr` are explicit.
- During optimization we track the value of `frame->instr_ptr`, emitting `_SET_IP` whenever `frame->instr_ptr` would have been updated.

The Tier 2 instruction pointer is strictly internal to the Tier 2 interpreter, so isn't visible to any other part of the code.

## Unwinding

Unwinding uses exception tables to find the next point at which normal execution can occur, or fail if there are no exception handlers. For more information on what exception tables are, see [exception handling](exception_handling.md).
During unwinding both the stack and the instruction pointer should be in their canonical, in-memory representation.

## Jumps in bytecode

The implementation of jumps within a single Tier 2 superblock/trace is just that, an implementation.
The implementation in the JIT and in the Tier 2 interpreter will necessarily be different.
What is in common is that representation in the Tier 2 optimizer.

We need the following types of jumps:

- Conditional branches within the superblock. These must only go forwards and be within the superblock.
- Terminal exits. These go back to the Tier 1 interpreter and cannot be modified.
- Loop end jumps. These go backwards, must be within the superblock, cannot be modified, and can only go to the start of the superblock.
- Patchable exits. These initially exit to code that tracks whether the exit is hot (presumably with a counter) and can be patched.

Currently, we don't have patchable exits.
Patching exits should be fairly straightforward in the interpreter.
It will be more complex in the JIT.

(We might also consider deoptimizations as a separate jump type.)
