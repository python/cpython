"""Utilities dealing with code objects."""

def compile_command(source, filename="<input>", symbol="single"):
    r"""Compile a command and determine whether it is incomplete.

    Arguments:

    source -- the source string; may contain \n characters
    filename -- optional filename from which source was read; default "<input>"
    symbol -- optional grammar start symbol; "single" (default) or "eval"

    Return value / exception raised:

    - Return a code object if the command is complete and valid
    - Return None if the command is incomplete
    - Raise SyntaxError if the command is a syntax error

    Approach:
    
    Compile three times: as is, with \n, and with \n\n appended.  If
    it compiles as is, it's complete.  If it compiles with one \n
    appended, we expect more.  If it doesn't compile either way, we
    compare the error we get when compiling with \n or \n\n appended.
    If the errors are the same, the code is broken.  But if the errors
    are different, we expect more.  Not intuitive; not even guaranteed
    to hold in future releases; but this matches the compiler's
    behavior in Python 1.4 and 1.5.

    """

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
    if not code1 and err1 == err2:
	raise SyntaxError, err1
