"""Utilities dealing with code objects."""

import sys
import string
import traceback

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


class InteractiveConsole:
    """Closely emulate the behavior of the interactive Python interpreter.

    After code by Jeff Epler and Fredrik Lundh.
    """

    def __init__(self, filename="<console>", locals=None):
        """Constructor.

        The optional filename argument specifies the (file)name of the
        input stream; it will show up in tracebacks.  It defaults to
        '<console>'.

        """
        self.filename = filename
        if locals is None:
            locals = {}
        self.locals = locals
        self.resetbuffer()

    def resetbuffer(self):
        """Reset the input buffer (but not the variables!)."""
        self.buffer = []

    def interact(self, banner=None):
        """Closely emulate the interactive Python console."""
        try:
            sys.ps1
        except AttributeError:
            sys.ps1 = ">>> "
        try:
            sys.ps2
        except AttributeError:
            sys.ps2 = "... "
        if banner is None:
            self.write("Python %s on %s\n%s\n(%s)\n" %
                       (sys.version, sys.platform, sys.copyright,
                        self.__class__.__name__))
        else:
            self.write("%s\n" % str(banner))
        more = 0
        while 1:
            try:
                if more:
                    prompt = sys.ps2
                else:
                    prompt = sys.ps1
                try:
                    line = self.raw_input(prompt)
                except EOFError:
                    self.write("\n")
                    break
                else:
                    more = self.push(line)
            except KeyboardInterrupt:
                self.write("\nKeyboardInterrupt\n")
                self.resetbuffer()
                more = 0

    def push(self, line):
        """Push a line to the interpreter.

        The line should not have a trailing newline.

        One of three things will happen:

        1) The input is incorrect; compile_command() raised
        SyntaxError.  A syntax traceback will be printed.

        2) The input is incomplete, and more input is required;
        compile_command() returned None.

        3) The input is complete; compile_command() returned a code
        object.  The code is executed.  When an exception occurs, a
        traceback is printed.  All exceptions are caught except
        SystemExit, which is reraised.

        The return value is 1 in case 2, 0 in the other cases.  (The
        return value can be used to decide whether to use sys.ps1 or
        sys.ps2 to prompt the next line.)

        A note about KeyboardInterrupt: this exception may occur
        elsewhere in this code, and will not always be caught.  The
        caller should be prepared to deal with it.

        """
        self.buffer.append(line)

        try:
            x = compile_command(string.join(self.buffer, "\n"),
                                filename=self.filename)
        except SyntaxError:
            # Case 1
            self.showsyntaxerror()
            self.resetbuffer()
            return 0

        if x is None:
            # Case 2
            return 1

        # Case 3
        try:
            exec x in self.locals
        except SystemExit:
            raise
        except:
            self.showtraceback()
        self.resetbuffer()
        return 0

    def showsyntaxerror(self):
        """Display the syntax error that just occurred.

        This doesn't display a stack trace because there isn't one.

        The output is written by self.write(), below.

        """
        type, value = sys.exc_info()[:2]
        # Work hard to stuff the correct filename in the exception
        try:
            msg, (filename, lineno, offset, line) = value
        except:
            pass
        else:
            try:
                value = SyntaxError(msg, (self.filename, lineno, offset, line))
            except:
                value = msg, (self.filename, lineno, offset, line)
        list = traceback.format_exception_only(type, value)
        map(self.write, list)

    def showtraceback(self):
        """Display the exception that just occurred.

        We remove the first stack item because it is our own code.

        The output is written by self.write(), below.

        """
        try:
            type, value, tb = sys.exc_info()
            tblist = traceback.extract_tb(tb)
            del tblist[0]
            list = traceback.format_list(tblist)
            list[len(list):] = traceback.format_exception_only(type, value)
        finally:
            tblist = tb = None
        map(self.write, list)

    def write(self, data):
        """Write a string.

        The base implementation writes to sys.stderr; a subclass may
        replace this with a different implementation.

        """
        sys.stderr.write(data)

    def raw_input(self, prompt=""):
        """Write a prompt and read a line.

        The returned line does not include the trailing newline.
        When the user enters the EOF key sequence, EOFError is raised.

        The base implementation uses the built-in function
        raw_input(); a subclass may replace this with a different
        implementation.

        """
        return raw_input(prompt)


def interact(banner=None, readfunc=None, locals=None):
    """Closely emulate the interactive Python interpreter.

    This is a backwards compatible interface to the InteractiveConsole
    class.  It attempts to import the readline module to enable GNU
    readline if it is available.

    Arguments (all optional, all default to None):

    banner -- passed to InteractiveConsole.interact()
    readfunc -- if not None, replaces InteractiveConsole.raw_input()
    locals -- passed to InteractiveConsole.__init__()

    """
    try:
        import readline
    except:
        pass
    console = InteractiveConsole(locals=locals)
    if readfunc is not None:
        console.raw_input = readfunc
    console.interact(banner)
                
if __name__ == '__main__':
    interact()
