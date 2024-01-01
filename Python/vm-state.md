# Python VM State

## Definition of Tiers

- **Tier 1** is the classic Python bytecode interpreter.
  This includes the specializing adaptive interpreter described in [PEP 659](https://peps.python.org/pep-0659/) and introduced in Python 3.11.
- **Tier 2**, also known as the micro-instruction ("uop") interpreter, is a new interpreter with a different instruction format.
  It will be introduced in Python 3.13, and also forms the basis for a JIT using copy-and-patch technology that is likely to be introduced at the same time (but, unlike the Tier 2 interpreter, hasn't landed in the main branch yet).

# Frame state

Almost all interpreter state is nominally stored in the frame structure.
A pointer to the current frame is held in `frame`. It contains:

- **local variables** (a.k.a. "fast locals")
- **evaluation stack** (tacked onto the end of the locals)
- **stack top** (an integer giving the top of the evaluation stack)
- **instruction pointer**
- **code object**, which holds things like the array of instructions, lists of constants and names referenced by certain instructions, the exception handling table, and the table that translates instruction offsets to line numbers
- **return offset**, only relevant during calls, telling the interpreter where to return

There are some other fields in the frame structure of less importance; notably frames are linked together in a singly-linked list via the `previous` pointer, pointing from callee to caller.
The frame also holds a pointer to the current function, globals, builtins, and the locals converted to dict (used to support the `locals()` built-in).

## Fast locals and evaluation stack

The frame contains a single array of object pointers, `localsplus`, which contains both the fast locals and the stack.
The top of the stack, including the locals, is indicated by `stacktop`.
For example, in a function with three locals, if the stack contains one value, `frame->stacktop == 4`.

The interpreters share an implementation which uses the same memory but caches the depth (as a pointer) in a C local, `stack_pointer`.
We aren't sure yet exactly how the JIT will implement the stack; likely some of the values near the top of the stack will be held in registers.

## Instruction pointer

The canonical, in-memory, representation of the instruction pointer is `frame->instr_ptr`.
It always points to an instruction in the bytecode array of the frame's code object.
Dispatching on `frame->instr_ptr` would be very inefficient, so in Tier 1 we cache the upcoming value of `frame->instr_ptr` in the C local `next_instr`.

## Tier 2

- `stack_pointer` is the same as in Tier 1 (but may be different in the JIT).
- At runtime we do not need a cache representation of `frame->instr_ptr`, as all stores to `frame->instr_ptr` are explicit.
- During optimization we track the value of `frame->instr_ptr`, emitting `_SET_IP` whenever `frame->instr_ptr` would have been updated.

The Tier 2 instruction pointer is strictly internal to the Tier 2 interpreter, so isn't visible to any other part of the code.

## Unwinding

Unwinding uses exception tables to find the next point at which normal execution can occur, or fail if there are no exception handlers.
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

# Thread state and interpreter state

Another important piece of VM state is the **thread state**, held in `tstate`.
The current frame pointer, `frame`, is always equal to `tstate->current_frame`.
The thread state also holds the exception state (`tstate->exc_info`) and the recursion counters (`tstate->c_recursion_remaining` and `tstate->py_recursion_remaining`).

The thread state is also used to access the **interpreter state** (`tstate->interp`), which is important since the "eval breaker" flags are stored there (`tstate->interp->ceval.eval_breaker`, an "atomic" variable), as well as the "PEP 523 function" (`tstate->interp->eval_frame`).
The interpreter state also holds the optimizer state (`optimizer` and some counters).
Note that the eval breaker may be moved to the thread state soon as part of the multicore (PEP 703) work.

# Tier 2 IR format

The tier 2 IR (Internal Representation) format is also the basis for the Tier 2 interpreter (though the two formats may eventually differ). This format is also used as the input to the machine code generator (the JIT compiler).

Tier 2 IR entries are all the same size; there is no equivalent to `EXTENDED_ARG` or trailing inline cache entries. Each instruction is a struct with the following fields (all integers of varying sizes):

- **opcode**: Sometimes the same as a Tier 1 opcode, sometimes a separate micro opcode. Tier 2 opcodes are 9 bits (as opposed to Tier 1 opcodes, which fit in 8 bits). By convention, Tier 2 opcode names start with `_`.
- **oparg**: The argument. Usually the same as the Tier 1 oparg after expansion of `EXTENDED_ARG` prefixes. Up to 32 bits.
- **operand**: An aditional argument, Typically the value of *one* cache item from the Tier 1 inline cache, up to 64 bits.
