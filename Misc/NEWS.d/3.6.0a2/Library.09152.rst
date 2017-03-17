signal, socket, and ssl module IntEnum constant name lookups now return a
consistent name for values having multiple names.  Ex: signal.Signals(6)
now refers to itself as signal.SIGALRM rather than flipping between that
and signal.SIGIOT based on the interpreter's hash randomization seed.