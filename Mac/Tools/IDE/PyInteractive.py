import string
import sys
import traceback


try:
    sys.ps1
except AttributeError:
    sys.ps1 = ">>> "
try:
    sys.ps2
except AttributeError:
    sys.ps2 = "... "


def print_exc(limit=None, file=None):
    if not file:
        file = sys.stderr
    # we're going to skip the outermost traceback object, we don't
    # want people to see the line which excecuted their code.
    tb = sys.exc_traceback
    if tb:
        tb = tb.tb_next
    try:
        sys.last_type = sys.exc_type
        sys.last_value = sys.exc_value
        sys.last_traceback = tb
        traceback.print_exception(sys.last_type, sys.last_value,
                                sys.last_traceback, limit, file)
    except:
        print '--- hola! ---'
        traceback.print_exception(sys.exc_type, sys.exc_value,
                                sys.exc_traceback, limit, file)


class PyInteractive:

    def __init__(self):
        import codeop
        self._pybuf = ""
        self._compile = codeop.Compile()

    def executeline(self, stuff, out = None, env = None):
        if env is None:
            import __main__
            env = __main__.__dict__
        if out:
            saveerr, saveout = sys.stderr, sys.stdout
            sys.stderr = sys.stdout = out
        try:
            if self._pybuf:
                self._pybuf = self._pybuf + '\n' + stuff
            else:
                self._pybuf = stuff

            # Compile three times: as is, with \n, and with \n\n appended.
            # If it compiles as is, it's complete.  If it compiles with
            # one \n appended, we expect more.  If it doesn't compile
            # either way, we compare the error we get when compiling with
            # \n or \n\n appended.  If the errors are the same, the code
            # is broken.  But if the errors are different, we expect more.
            # Not intuitive; not even guaranteed to hold in future
            # releases; but this matches the compiler's behavior in Python
            # 1.4 and 1.5.
            err = err1 = err2 = None
            code = code1 = code2 = None

            # quickly get out of here when the line is 'empty' or is a comment
            stripped = string.strip(self._pybuf)
            if not stripped or stripped[0] == '#':
                self._pybuf = ''
                sys.stdout.write(sys.ps1)
                sys.stdout.flush()
                return

            try:
                code = self._compile(self._pybuf, "<input>", "single")
            except SyntaxError, err:
                pass
            except:
                # OverflowError. More?
                print_exc()
                self._pybuf = ""
                sys.stdout.write(sys.ps1)
                sys.stdout.flush()
                return

            try:
                code1 = self._compile(self._pybuf + "\n", "<input>", "single")
            except SyntaxError, err1:
                pass

            try:
                code2 = self._compile(self._pybuf + "\n\n", "<input>", "single")
            except SyntaxError, err2:
                pass

            if code:
                try:
                    exec code in env
                except:
                    print_exc()
                self._pybuf = ""
            elif code1:
                pass
            elif err1 == err2 or (not stuff and self._pybuf):
                print_exc()
                self._pybuf = ""
            if self._pybuf:
                sys.stdout.write(sys.ps2)
                sys.stdout.flush()
            else:
                sys.stdout.write(sys.ps1)
                sys.stdout.flush()
        finally:
            if out:
                sys.stderr, sys.stdout = saveerr, saveout
