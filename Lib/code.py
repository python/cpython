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


def interact(banner=None, readfunc=raw_input, local=None):
    # Due to Jeff Epler, with changes by Guido:
    """Closely emulate the interactive Python console."""
    try: import readline # Enable GNU readline if available
    except: pass
    local = local or {}
    import sys, string, traceback
    sys.ps1 = '>>> '
    sys.ps2 = '... '
    if banner:
	print banner
    else:
	print "Python Interactive Console", sys.version
	print sys.copyright
    buf = []
    while 1:
	if buf: prompt = sys.ps2
	else: prompt = sys.ps1
	try: line = readfunc(prompt)
	except KeyboardInterrupt:
	    print "\nKeyboardInterrupt"
	    buf = []
	    continue
	except EOFError: break
	buf.append(line)
	try: x = compile_command(string.join(buf, "\n"))
	except SyntaxError:
	    traceback.print_exc(0)
	    buf = []
	    continue
	if x == None: continue
	else:
	    try: exec x in local
	    except:
		exc_type, exc_value, exc_traceback = \
			sys.exc_type, sys.exc_value, \
			sys.exc_traceback
		l = len(traceback.extract_tb(sys.exc_traceback))
		try: 1/0
		except:
		    m = len(traceback.extract_tb(
			    sys.exc_traceback))
		traceback.print_exception(exc_type,
			exc_value, exc_traceback, l-m)
	    buf = []
		
if __name__ == '__main__':
    interact()
