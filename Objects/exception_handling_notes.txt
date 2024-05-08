Description of exception handling in Python 3.11
------------------------------------------------

Python 3.11 uses what is known as "zero-cost" exception handling.
Prior to 3.11, exceptions were handled by a runtime stack of "blocks".

In zero-cost exception handling, the cost of supporting exceptions is minimized.
In the common case (where no exception is raised) the cost is reduced
to zero (or close to zero).
The cost of raising an exception is increased, but not by much.

The following code:

def f():
    try:
        g(0)
    except:
        return "fail"

compiles as follows in 3.10:

  2           0 SETUP_FINALLY            7 (to 16)

  3           2 LOAD_GLOBAL              0 (g)
              4 LOAD_CONST               1 (0)
              6 CALL_NO_KW               1
              8 POP_TOP
             10 POP_BLOCK
             12 LOAD_CONST               0 (None)
             14 RETURN_VALUE

  4     >>   16 POP_TOP
             18 POP_TOP
             20 POP_TOP

  5          22 POP_EXCEPT
             24 LOAD_CONST               3 ('fail')
             26 RETURN_VALUE

Note the explicit instructions to push and pop from the "block" stack:
SETUP_FINALLY and POP_BLOCK.

In 3.11, the SETUP_FINALLY and POP_BLOCK are eliminated, replaced with
a table to determine where to jump to when an exception is raised.

  1           0 RESUME                   0

  2           2 NOP

  3           4 LOAD_GLOBAL              1 (g + NULL)
             16 LOAD_CONST               1 (0)
             18 PRECALL                  1
             22 CALL                     1
             32 POP_TOP
             34 LOAD_CONST               0 (None)
             36 RETURN_VALUE
        >>   38 PUSH_EXC_INFO

  4          40 POP_TOP

  5          42 POP_EXCEPT
             44 LOAD_CONST               2 ('fail')
             46 RETURN_VALUE
        >>   48 COPY                     3
             50 POP_EXCEPT
             52 RERAISE                  1
ExceptionTable:
  4 to 32 -> 38 [0]
  38 to 40 -> 48 [1] lasti

(Note this code is from 3.11, later versions may have slightly different bytecode.)

If an instruction raises an exception then its offset is used to find the target to jump to.
For example, the CALL at offset 22, falls into the range 4 to 32.
So, if g() raises an exception, then control jumps to offset 38.


Unwinding
---------

When an exception is raised, the current instruction offset is used to find following:
target to jump to, stack depth, and 'lasti', which determines whether the instruction
offset of the raising instruction should be pushed.

This information is stored in the exception table, described below.

If there is no relevant entry, the exception bubbles up to the caller.

If there is an entry, then:
 1. pop values from the stack until it matches the stack depth for the handler.
 2. if 'lasti' is true, then push the offset that the exception was raised at.
 3. push the exception to the stack.
 4. jump to the target offset and resume execution.


Format of the exception table
-----------------------------

Conceptually, the exception table consists of a sequence of 5-tuples:
    1. start-offset (inclusive)
    2. end-offset (exclusive)
    3. target
    4. stack-depth
    5. push-lasti (boolean)

All offsets and lengths are in instructions, not bytes.

We want the format to be compact, but quickly searchable.
For it to be compact, it needs to have variable sized entries so that we can store common (small) offsets compactly, but handle large offsets if needed.
For it to be searchable quickly, we need to support binary search giving us log(n) performance in all cases.
Binary search typically assumes fixed size entries, but that is not necessary, as long as we can identify the start of an entry.

It is worth noting that the size (end-start) is always smaller than the end, so we encode the entries as:
    start, size, target, depth, push-lasti

Also, sizes are limited to 2**30 as the code length cannot exceed 2**31 and each instruction takes 2 bytes.
It also happens that depth is generally quite small.

So, we need to encode:
    start (up to 30 bits)
    size (up to 30 bits)
    target (up to 30 bits)
    depth (up to ~8 bits)
    lasti (1 bit)

We need a marker for the start of the entry, so the first byte of entry will have the most significant bit set.
Since the most significant bit is reserved for marking the start of an entry, we have 7 bits per byte to encode offsets.
Encoding uses a standard varint encoding, but with only 7 bits instead of the usual 8.
The 8 bits of a bit are (msb left) SXdddddd where S is the start bit. X is the extend bit meaning that the next byte is required to extend the offset.

In addition, we will combine depth and lasti into a single value, ((depth<<1)+lasti), before encoding.

For example, the exception entry:
    start:  20
    end:    28
    target: 100
    depth:  3
    lasti:  False

is encoded first by converting to the more compact four value form:
    start:         20
    size:          8
    target:        100
  depth<<1+lasti:  6

which is then encoded as:
    148 (MSB + 20 for start)
    8   (size)
    65  (Extend bit + 1)
    36  (Remainder of target, 100 == (1<<6)+36)
    6

for a total of five bytes.



Script to parse the exception table
-----------------------------------

def parse_varint(iterator):
    b = next(iterator)
    val = b & 63
    while b&64:
        val <<= 6
        b = next(iterator)
        val |= b&63
    return val

def parse_exception_table(code):
    iterator = iter(code.co_exceptiontable)
    try:
        while True:
            start = parse_varint(iterator)*2
            length = parse_varint(iterator)*2
            end = start + length - 2 # Present as inclusive, not exclusive
            target = parse_varint(iterator)*2
            dl = parse_varint(iterator)
            depth = dl >> 1
            lasti = bool(dl&1)
            yield start, end, target, depth, lasti
    except StopIteration:
        return
