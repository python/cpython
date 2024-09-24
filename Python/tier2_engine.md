# The tier 2 execution engine

## General idea

When execution in tier 1 becomes "hot", that is the counter for that point in
the code reaches some threshold, we create an executor and execute that
instead of the tier 1 bytecode.

Since each executor must exit, we also track the "hotness" of those
exits and attach new executors to those exits.

As the program executes, and the hot parts of the program get optimized,
a graph of executors forms.

## Superblocks and Executors

Once a point in the code has become hot enough, we want to optimize it.
Starting from that point we project the likely path of execution,
using information gathered by tier 1 to guide that projection to
form a "superblock", a mostly linear sequence of micro-ops.
Although mostly linear, it may include a single loop.

We then optimize this superblock to form an optimized superblock,
which is equivalent but more efficient.

A superblock is a representation of the code we want to execute,
but it is not in executable form.
The executable form is known as an executor.

Executors are semantically equivalent to the superblock they are
created from, but are in a form that can be efficiently executable.

There are two execution engines for executors, and two types of executors:
* The hardware which runs machine code executors created by the JIT compiler.
* The tier 2 interpreter runs bytecode executors.

It would be very wasteful to support both a tier 2 interpreter and
JIT compiler in the same process.
For now, we will make the choice of engine a configuration option,
but we could make it a command line option in the future if that would prove useful.


### Tier 2 Interpreter

For platforms without a JIT and for testing, we need an interpreter
for executors. It is similar in design to the tier 1 interpreter, but has a
different instruction set, and does not adapt.

### JIT compiler

The JIT compiler converts superblocks into machine code executors.
These have identical behavior to interpreted executors, except that
they consume more memory for the generated machine code and are a lot faster.

## Transferring control

There are three types of control transfer that we need to consider:
* Tier 1 to tier 2
* Tier 2 to tier 1
* One executor to another within tier 2

Since we expect the graph of executors to span most of the hot
part of the program, transfers from one executor to another should
be the most common.
Therefore, we want to make those transfers fast.

### Tier 2 to tier 2

#### Cold exits

All side exits start cold and most stay cold, but a few become
hot. We want to keep the memory consumption small for the many
cold exits, but those that become hot need to be fast.
However we cannot know in advance, which will be which.

So that tier 2 to tier 2 transfers are fast for hot exits,
exits must be implemented as executors. In order to patch
executor exits when they get hot, a pointer to the current
executor must be passed to the exit executor.

#### Handling reference counts

There must be an implicit reference to the currently executing
executor, otherwise it might be freed.
Consequently, we must increment the reference count of an
executor just before executing it, and decrement it just after
executing it.

We want to minimize the amount of data that is passed from
one executor to the next. In the JIT, this reduces the number
of arguments in the tailcall, freeing up registers for other uses.
It is less important in the interpreter, but following the same
design as the JIT simplifies debugging and is good for performance.

Provided that we incref the new executor before executing it, we
can jump directly to the code of the executor, without needing
to pass a reference to that executor object.
However, we do need a reference to the previous executor,
so that it can be decref'd and for handling of cold exits.
To avoid messing up the JIT's register allocation, we pass a
reference to the previous executor in the thread state's
`previous_executor` field.

#### The interpreter

The tier 2 interpreter has a variable `current_executor` which
points to the currently live executor. When transferring from executor
`A` to executor `B` we do the following:
(Initially `current_executor` points to `A`, and the refcount of
`A` is elevated by one)

1. Set the instruction pointer to start at the beginning of `B`
2. Increment the reference count of `B`
3. Start executing `B`

We also make the first instruction in `B` do the following:
1. Set `current_executor` to point to `B`
2. Decrement the reference count of `A` (`A` is referenced by `tstate->previous_executor`)

The net effect of the above is to safely decrement the refcount of `A`,
increment the refcount of `B` and set `current_executor` to point to `B`.

#### In the JIT

Transferring control from one executor to another is done via tailcalls.

The compiled executor should do the same, except that there is no local
variable `current_executor`.

### Tier 1 to tier 2

Since the executor doesn't know if the previous code was tier 1 or tier 2,
we need to make a transfer from tier 1 to tier 2 look like a tier 2 to tier 2
transfer to the executor.

We can then perform a tier 1 to tier 2 transfer by setting `current_executor`
to `None`, and then performing a tier 2 to tier 2 transfer as above.

### Tier 2 to tier 1

Each micro-op that might exit to tier 1 contains a `target` value,
which is the offset of the tier 1 instruction to exit to in the
current code object.

## Counters

TO DO.
The implementation will change soon, so there is no point in
documenting it until then.

