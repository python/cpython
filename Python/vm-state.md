# Definition of Tiers

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
Dispatching on `frame->instr_ptr` would be very inefficient, so in Tier 1 we cache the upcoming value of `frame->instr_ptr` in the C local `next_instr`.

### Tier 2

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
- Terminal exits. These go back to the Tier 1 interpreter and cannot be modified
- Loop end jumps. These go backwards, must be within the superblock, cannot be modified, and can only go to the start of the superblock.
- Patchable exits. These initially exit to code that tracks whether the exit is hot (presumably with a counter) and can be patched.

Currently, we don't have patchable exits.
Patching exits should be fairly straightforward in the interpreter.
It will be more complex in the JIT.

# Thread state and interpreter state

Another important piece of state is the **thread state**, held in `tstate`.
The current frame pointer, `frame`, is always equal to `tstate->current_frame`.
The thread state also holds the exception state (`tstate->exc_info`) and the recursion counters (`tstate->c_recursion_remaining` and `tstate->py_recursion_remaining`).

The thread state is also used to access the **interpreter state** (`tstate->interp`), which is important since the "eval breaker" flags are stored there (`tstate->interp->ceval.eval_breaker`, an "atomic" variable), as well as the "PEP 523 function" (`tstate->interp->eval_frame`).
The interpreter state also holds the optimizer state (`optimizer` and some counters).
Note that the eval breaker may be moved to the thread state soon as part of the multicore (PEP 703) work.

# Tier 2 considerations

The Tier 2 interpreter uses `frame` and `tstate` exactly as the Tier 1 interpreter.

It happens to use `stack_pointer` the same way as well, but by convention when switching between tiers the stack pointer is stored and reloaded using the `STORE_SP()` and `LOAD_SP()` macros (it just so happens that they share the local variable `stack_pointer`, and the sequence `STORE_SP(); LOAD_SP()` is effectively a no-op, even preserving the invariant that when the stack pointer is held in `stack_pointer`, `frame->stacktop` is invalidated by storing `-1` there).

The instruction pointer is handled entirely differently between the tiers. In Tier 1, `next_instr` is used to decode each instruction, and also used to access inline cache values. Whenever an instruction is decoded, `frame->instr_ptr` is made to point to the start of the instruction (excluding `EXTENDED_ARG` prefixes -- the important invariant is that `frame->instr_ptr->op.code` is the opcode of the current instruction). Simultaneously, `next_instr` is incremented so that it either points at the *next* instruction (for simple instructions), or at the first in-line cache entry for the current instruction. In-line cache values are accessed as `next_instr[0].cache`, `next_instr[1].cache`, etc. (For cache values larger than 16 bits, helper functions like `read_u32()` exist.)

However, in Tier 2, there are actually *two* "instruction pointers": First, there is a pointer to the (current or next) Tier 2 micro-op. This tells the Tier 2 interpreter the location of the next micro-instruction. Separately, the Tier 1 instruction pointer must still be available, because it (or, actually, the corresponding index into the Tier 1 bytecode array) is used for exception handling. It is used by the exception handling table to find the exception handler if an exception occurs, and to report the line and column numbers if a traceback is produced. By convention, these processes use the instruction pointer from `frame->instr_ptr`, not the contents of the (only roughly corresponding) `next_instr` variable.

The translation process from Tier 1 bytecode to Tier 2 micro-ops outputs a `_SET_IP` micro-op at the start of each translated bytecode. This is a special uop that directs the Tier 2 interpreter to store a specific value into `frame->instr_ptr`. Because this produces a *lot* of `_SET_IP` uops, a quick post-processing pass removes `_SET_IP` uops whose original Tier 1 bytecode cannot produce an exception (e.g., `LOAD_FAST` or `STORE_FAST`).

# JIT considerations

The copy-and-patch JIT is considered a variant of the Tier 2 interpreter. The `frame`, `stack_pointer` and `tstate` values are treated exactly the same. This means that it also automatically updates `frame->instr_ptr` the same way whenever it executes a `_SET_IP` uop. The only difference is that in the JIT world, the array of Tier 2 uops is irrelevant, and there is no Tier 2 instruction pointer. Instead, there is just the hardware CPU's program counter.

There are a few complications that will probably change:

- For conditional jump instructions, the JIT needs to detect whether the instruction actually jumped. This is currently done through a hack in the JIT instruction template that observes whether the Tier 2 interpreter's "program counter" was changed, and then jumps (using the lucky invariant that in Tier 2 the index of the jump target instruction is always equal to `oparg`). Instead, we should introduce a macro that in the Tier 2 interpreter that updates the Tier 2 program counter (the Tier 1 program counter will be taken care by an explicit `_SET_IP` uop at the jump target), whereas in the JIT the macro will just set a flag that is tested by the JIT template. Note that for uops that can't jump, the C compiler will optimize away this flag and its testing (as it does for the current hack).
- The `_SET_IP` uop currently uses a helper variable `ip_offset` which is set to point to the start of the bytecode array whenever the Tier 2 interpreter switches frames (i.e., at the top, and in the `_PUSH_FRAME` and `_POP_FRAME` uops). We should get rid of the helper variable and just replace its (few) uses with `_PyCode_CODE(_PyFrame_GetCode(frame))`.

# Tier 2 IR format

The tier 2 IR (Internal Representation) format is also the basis for the Tier 2 interpreter (though the two formats may eventually differ). This format is also used as the input to the machine code generator (the JIT compiler).

Tier 2 IR entries are all the same size; there is no equivalent to `EXTENDED_ARG` or trailing inline cache entries. Each instruction is a struct with the following fields (all integers of varying sizes):

- **opcode**: Sometimes the same as a Tier 1 opcode, sometimes a separate micro opcode. Tier 2 opcodes are 9 bits (as opposed to Tier 1 opcodes, which fit in 8 bits). By convention, Tier 2 opcode names start with `_`.
- **oparg**: The argument. Usually the same as the Tier 1 oparg after expansion of `EXTENDED_ARG` prefixes. Up to 32 bits.
- **operand**: An aditional argument, Typically the value of *one* cache item from the Tier 1 inline cache, up to 64 bits.
