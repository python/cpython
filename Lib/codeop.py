"""Utility to compile possibly incomplete Python source code."""

import sys
import string
import traceback

def compile_command(source, filename="<input>", symbol="single"):
    r"""Compile a command and determine whether it is incomplete.

    Arguments:

    source -- the source string; may contain \n characters
    filename -- optional filename from which source was read; default "<input>"
    symbol -- optional grammar start symbol; "single" (default) or "eval"

    Return value / exceptions raised:

    - Return a code object if the command is complete and valid
    - Return None if the command is incomplete
    - Raise SyntaxError or OverflowError if the command is a syntax error
      (OverflowError if the error is in a numeric constant)

    Approach:

    First, check if the source consists entirely of blank lines and
    comments; if so, replace it with 'pass', because the built-in
    parser doesn't always do the right thing for these.

    Compile three times: as is, with \n, and with \n\n appended.  If
    it compiles as is, it's complete.  If it compiles with one \n
    appended, we expect more.  If it doesn't compile either way, we
    compare the error we get when compiling with \n or \n\n appended.
    If the errors are the same, the code is broken.  But if the errors
    are different, we expect more.  Not intuitive; not even guaranteed
    to hold in future releases; but this matches the compiler's
    behavior from Python 1.4 through 1.5.2, at least.

    Caveat:

    It is possible (but not likely) that the parser stops parsing
    with a successful outcome before reaching the end of the source;
    in this case, trailing symbols may be ignored instead of causing an
    error.  For example, a backslash followed by two newlines may be
    followed by arbitrary garbage.  This will be fixed once the API
    for the parser is better.

    """

    # Check for source consisting of only blank lines and comments
    for line in string.split(source, "\n"):
        line = string.strip(line)
        if line and line[0] != '#':
            break               # Leave it alone
    else:
        source = "pass"         # Replace it with a 'pass' statement

    err = err1 = err2 = None
    code = code1 = code2 = None

    try:
        code = compile(source, filename, symbol)
    except SyntaxError, err:
        pass

    try:
        code1 = compile(source + "\n", filename, symbol)
    except SyntaxError, err1:
        pass

    try:
        code2 = compile(source + "\n\n", filename, symbol)
    except SyntaxError, err2:
        pass

    if code:
        return code
    try:
        e1 = err1.__dict__
    except AttributeError:
        e1 = err1
    try:
        e2 = err2.__dict__
    except AttributeError:
        e2 = err2
    if not code1 and e1 == e2:
        raise SyntaxError, err1
