"""
The regular expression engine in '_sre' can segfault when interpreting
bogus bytecode.

It is unclear whether this is a real bug or a "won't fix" case like
bogus_code_obj.py, because it requires bytecode that is built by hand,
as opposed to compiled by 're' from a string-source regexp.  The
difference with bogus_code_obj, though, is that the only existing regexp
compiler is written in Python, so that the C code has no choice but
accept arbitrary bytecode from Python-level.

The test below builds and runs random bytecodes until 'match' crashes
Python.  I have not investigated why exactly segfaults occur nor how
hard they would be to fix.  Here are a few examples of 'code' that
segfault for me:

    [21, 50814, 8, 29, 16]
    [21, 3967, 26, 10, 23, 54113]
    [29, 23, 0, 2, 5]
    [31, 64351, 0, 28, 3, 22281, 20, 4463, 9, 25, 59154, 15245, 2,
                  16343, 3, 11600, 24380, 10, 37556, 10, 31, 15, 31]

Here is also a 'code' that triggers an infinite uninterruptible loop:

    [29, 1, 8, 21, 1, 43083, 6]

"""

import _sre, random

def pick():
    n = random.randrange(-65536, 65536)
    if n < 0:
        n &= 31
    return n

ss = ["", "world", "x" * 500]

while 1:
    code = [pick() for i in range(random.randrange(5, 25))]
    print code
    pat = _sre.compile(None, 0, code)
    for s in ss:
        try:
            pat.match(s)
        except RuntimeError:
            pass
