# The tier 2 execution engine

## General idea

When execution in tier 1 becomes "hot", that is the counter for that point in
the code reaches some threshold, we create an executor and execute that
instead of the tier 1 bytecode.

Since each executor must exit, we also track the "hotness" of those
exits and attach new executors to those exits.

That way we form a graph of executors that covers the most frequently
executed parts of the program.

## Superblocks and Executors

Once a point in the code has become hot enough, we want to optimize it.
Starting from that point we project the likely path of execution,
using information gathered by tier 1 to guide that projection to
form a "superblock", a mostly linear sequence of micro-ops.
Although mostly linear, it may include a single loop.

We then optimize this superblock to from an optimized superblock,
which is equivalent but more efficient.

A superblock is a representation of the code we want to execute,
but it is not in executable form.
The executable form is know as an executor.

Executors are semantically equivalent to the superblock they are
created from, but are in a form that can be efficiently executable.

There are two execution engines for executors, adn two types of executors:
* The hardware which runs machine code executors created by the JIT compiler.
* The tier 2 interpreter runs bytecode executors.

The choice of engine is a configuration option.
We will not support both the tier 2 interpreter and JIT in a
single executable.

### Tier 2 Interpreter

For platforms without a JIT and for testing, we need an interpreter
for executors. It is similar in design to the tier 1 interpreter, but has a
different instruction set, and does not adapt.

### JIT compiler

The JIT compiler converts superblocks into machine code executors.
These have identical behavior to interpreted executors, except that
they consume a bit more memory and are a lot faster.

## Transfering control

There are three types of control transfer that we need to consider:
* Tier 1 to tier 2
* Tier 2 to tier 1
* One executor to another within tier 2

Since we expect the graph of executors to span most of the hot
part of the program, transfers from one executor to another should
be the most common.
Therefore, we want to make those transfers fast.

### Tier 2 to tier 2

#### Handling reference counts

There must be an implicit reference to the currently executing
executor, otherwise it might be freed.
Consequently, we must increment the reference count of an
executor just before executing it, and decrement it just after
executing it.

Note that since executors are objects, they can contain references to
themselves, which means we do not need to pass a reference to an
executor when we start to execute it.
However, we do need to pass a reference to the previous executor,
so that it can be decref'd.

#### The interpreter

The tier 2 interpreter has a variable `current_executor` which
points to the currently live executor. When transfering from executor
`A` to executor `B` we do the following:
(Initially `current_executor` points to `A`, and the refcount of
`A` is elevated by one)

1. Set the instruction pointer to start at the beginning of `B`
2. Increment the reference count of `B`
3. Start executing `B`

We also make the first instruction in `B` do the following:
1. Decrement the reference count of `A` (through `current_executor`)
2. Set `current_executor` to point to `B`

The net effect of the above is to safely decrement the refcount of `A`,
increment the refcount of `B` and set `current_executor` to point to `B`.

#### In the JIT

Transfering control form one executor to another is done via tailcalls.

The compiled executor should do the same, except that there is no local
variable `current_executor`, so that the old executor, `A`, must be passed
as an additional parameter when tailcalling.

### Tier 1 to tier 2

We create a single, immortal executor that cannot be executed.
We can then perform a tier 1 to tier 2 transfer,
by setting `current_executor` to this singleton, and then performing
a tier 2 to tier 2 transfer as above.

### Tier 2 to tier 1

Each micro-op that might exit to tier 1 contains a `target` value,
which is the offset of the tier 1 instruction to exit to in the
current code object.

## Counters

TO DO.
The implementation will change soon, so there is no point in
documenting it until then.

