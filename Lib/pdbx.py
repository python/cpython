import abc
import argparse
import cmd
import io
import linecache
import os
import pprint
import runpy
import sys
import traceback

from bdbx import Bdbx, BdbxQuit
from types import CodeType


class _ExecuteTarget(abc.ABC):
    code: CodeType

    @abc.abstractmethod
    def __init__(self, filename: str):
        pass

    @property
    @abc.abstractmethod
    def namespaces(self) -> tuple[dict, dict]:
        pass


class _ScriptTarget(_ExecuteTarget):
    def __init__(self, filename):
        self.filename = filename
        if not os.path.exists(filename):
            raise FileNotFoundError(filename)
        with io.open_code(filename) as f:
            self.code = compile(f.read(), filename, "exec")

    @property
    def namespaces(self):
        ns = {
            '__name__': '__main__',
            '__file__': self.filename,
            '__builtins__': __builtins__,
        }
        return ns, ns


class _ModuleTarget(_ExecuteTarget):
    def __init__(self, filename):
        # Just raise the normal exception if module is not found
        _, self.spec, self.code = runpy._get_module_details(filename)
        self.filename = os.path.normcase(os.path.abspath(self.code.co_filename))

    @property
    def namespaces(self):
        ns = {
            '__name__': '__main__',
            '__file__': self.filename,
            '__package__': self.spec.parent,
            '__loader__': self.spec.loader,
            '__spec__': self.spec,
            '__builtins___': __builtins__,
        }
        return ns, ns

class Pdbx(Bdbx, cmd.Cmd):
    def __init__(self):
        self._event = None
        self.prompt = "(Pdbx) "
        self.fncache = {}
        Bdbx.__init__(self)
        cmd.Cmd.__init__(self, 'tab', None, None)

    # ========================================================================
    # ============================ User APIs =================================
    # ========================================================================


    # ========================================================================
    # ======================= Interface to Bdbx ==============================
    # ========================================================================

    def dispatch_event(self, event):
        self._event = event
        self.print_header()
        self.cmdloop()

    # ========================================================================
    # ================= Methods that can be overwritten ======================
    # ========================================================================

    def error(self, msg):
        print(msg)

    def message(self, msg):
        print(msg)

    def print_header(self):
        if self._event.is_call:
            print("----call----")
        elif self._event.is_return:
            print("----return----")
        if self._event.line_number:
            lineno = self._event.line_number
        else:
            lineno = self._event.frame.f_lineno
        self._print_stack_entry(self._event.frame, lineno)

    # ========================================================================
    # ======================== helper functions ==============================
    # ========================================================================

    def _run_target(self, target:_ExecuteTarget):
        """Debug the given code object in __main__"""
        import __main__
        main_dict = __main__.__dict__.copy()
        globals, locals = target.namespaces
        __main__.__dict__.clear()
        __main__.__dict__.update(globals)
        self.break_code(target.code)
        exec(target.code, globals, locals)
        __main__.__dict__.clear()
        __main__.__dict__.update(main_dict)

    @property
    def _default_file(self):
        """Produce a reasonable default."""
        filename = self.get_current_frame().f_code.co_filename
        if filename == '<string>' and self._main_pyfile:
            filename = self._main_pyfile
        return filename

    # ======================= Formatting Helpers ============================
    def _format_stack_entry(self, frame, lineno, stack_prefix="> ", code_prefix="-> "):
        """Return a string with information about a stack entry.

        The stack entry frame_lineno is a (frame, lineno) tuple.  The
        return string contains the canonical filename, the function name
        or '<lambda>', the input arguments, the return value, and the
        line of code (if it exists).

        """
        filename = self._canonic(frame.f_code.co_filename)
        s = '%s(%r)' % (filename, lineno)
        if frame.f_code.co_name:
            func_name = frame.f_code.co_name
        else:
            func_name = "<lambda>"

        code = linecache.getline(filename, lineno, frame.f_globals)
        if code:
            code = f"\n{code_prefix}{code.strip()}"
        else:
            code = ""
        return f"{stack_prefix}{func_name}() @ {filename}:{lineno}{code}"

    def _format_exception(exc):
        return traceback.format_exception_only(exc)[-1].strip()

    def _print_stack_entry(self,
                           frame,
                           line_number=None,
                           stack_prefix=None,
                           code_prefix=None):
        if line_number is None:
            line_number = frame.f_lineno
        if stack_prefix is None:
            if frame is self.get_current_frame():
                stack_prefix = '> '
            else:
                stack_prefix = '  '
        if code_prefix is None:
            code_prefix = '-> '
        self.message(self._format_stack_entry(frame,
                                              line_number,
                                              stack_prefix=stack_prefix,
                                              code_prefix=code_prefix))

    def _canonic(self, filename):
        """Return canonical form of filename.

        For real filenames, the canonical form is a case-normalized (on
        case insensitive filesystems) absolute path.  'Filenames' with
        angle brackets, such as "<stdin>", generated in interactive
        mode, are returned unchanged.
        """
        if filename == "<" + filename[1:-1] + ">":
            return filename
        canonic = self.fncache.get(filename)
        if not canonic:
            canonic = os.path.abspath(filename)
            canonic = os.path.normcase(canonic)
            self.fncache[filename] = canonic
        return canonic

    def _lookupmodule(self, filename):
        """Helper function for break/clear parsing -- may be overridden.

        lookupmodule() translates (possibly incomplete) file or module name
        into an absolute file name.
        """
        if os.path.isabs(filename) and os.path.exists(filename):
            return filename
        f = os.path.join(sys.path[0], filename)
        if os.path.exists(f) and self._canonic(f) == self._main_pyfile:
            return f
        root, ext = os.path.splitext(filename)
        if ext == '':
            filename = filename + '.py'
        if os.path.isabs(filename):
            return filename
        for dirname in sys.path:
            while os.path.islink(dirname):
                dirname = os.readlink(dirname)
            fullname = os.path.join(dirname, filename)
            if os.path.exists(fullname):
                return fullname
        return None

    def _confirm_line_executable(self, filename, line_number):
        """Check whether specified line seems to be executable.

        Return `lineno` if it is, 0 if not (e.g. a docstring, comment, blank
        line or EOF). Warning: testing is not comprehensive.
        """
        # this method should be callable before starting debugging, so default
        # to "no globals" if there is no current frame
        globs = self.get_current_frame().f_globals if self.get_current_frame() else None
        line = linecache.getline(filename, line_number, globs)
        if not line:
            raise ValueError(f"Can't find line {line_number} in file {filename}")
        line = line.strip()
        # Don't allow setting breakpoint at a blank line
        if (not line or (line[0] == '#') or
             (line[:3] == '"""') or line[:3] == "'''"):
            raise ValueError(f"Can't set breakpoint at line {line_number} in file {filename}, it's blank or a comment")

    def _parse_breakpoint_arg(self, arg):
        """Parse a breakpoint argument.

        Return a tuple (filename, lineno, function, condition)
        from [filename:]lineno | function, condition
        """
        filename = None
        line_number = None
        function = None
        condition = None
        # parse stuff after comma: condition
        comma = arg.find(',')
        if comma >= 0:
            condition = arg[comma+1:].lstrip()
            arg = arg[:comma]

        # parse stuff before comma: [filename:]lineno | function
        colon = arg.rfind(':')
        if colon >= 0:
            filename = arg[:colon].rstrip()
            filename = self._lookupmodule(filename)
            if filename is None:
                raise ValueError(f"Invalid filename: {filename}")
            line_number = arg[colon+1:]
            try:
                line_number = int(line_number)
            except ValueError:
                raise ValueError(f"Invalid line number: {line_number}")
        else:
            # no colon; can be lineno or function
            try:
                # Maybe it's a function?
                frame = self.get_current_frame()
                function = eval(arg,
                                frame.f_globals,
                                frame.f_locals)
                if hasattr(function, "__func__"):
                    # It's a method
                    function = function.__func__
                code = function.__code__
                filename = code.co_filename
                line_number = code.co_firstlineno
            except:
                # Then it has to be a line number
                function = None
                try:
                    line_number = int(arg)
                except ValueError:
                    raise ValueError(f"Invalid breakpoint argument: {arg}")
                filename = self._default_file
        # Before returning, check that line on the file is executable
        self._confirm_line_executable(filename, line_number)

        return filename, line_number, function, condition

    def _getval(self, arg):
        try:
            frame = self.get_current_frame()
            return eval(arg, frame.f_globals, frame.f_locals)
        except:
            self.error(self._format_exception(sys.exception()))
            raise

    # ======================================================================
    # The following methods are called by the cmd.Cmd base class
    # All the commands are in alphabetic order
    # ======================================================================

    def do_break(self, arg):
        if not arg:
            for bp in self.get_breakpoints():
                print(bp)
            return False
        try:
            filename, line_number, function, condition = self._parse_breakpoint_arg(arg)
        except ValueError as exc:
            self.error(str(exc))
            return False

        if function:
            self.set_function_breakpoint(function)
        else:
            self.set_file_breakpoint(filename, line_number)
        return False

    do_b = do_break

    def do_clear(self, arg):
        self.clear_breakpoints()
        return False

    def do_continue(self, arg):
        self.set_action("continue")
        return True

    do_c = do_continue

    def do_down(self, arg):
        """d(own) [count]

        Move the current frame count (default one) levels down in the
        stack trace (to a newer frame).
        """
        try:
            count = int(arg or 1)
        except ValueError:
            self.error("Invalid count")
            return False

        self.select_frame(count, offset=True)
        self._print_stack_entry(self.get_current_frame())
        return False

    do_d = do_down

    def do_EOF(self, arg):
        self.message('')
        raise BdbxQuit("quit")

    def do_next(self, arg):
        self.set_action("next")
        return True

    do_n = do_next

    def do_p(self, arg):
        """p expression

        Print the value of the expression.
        """
        try:
            val = self._getval(arg)
            self.message(repr(val))
        except:
            # error message is printed
            pass
        return False

    def do_pp(self, arg):
        """pp expression

        Pretty-print the value of the expression.
        """
        try:
            val = self._getval(arg)
            self.message(pprint.pformat(val))
        except:
            # error message is printed
            pass
        return False

    def do_quit(self, arg):
        raise BdbxQuit("quit")

    do_q = do_quit

    def do_return(self, arg):
        self.set_action("return")
        return True

    do_r = do_return

    def do_step(self, arg):
        self.set_action("step")
        return True

    do_s = do_step

    def do_up(self, arg):
        """u(p) [count]

        Move the current frame count (default one) levels up in the
        stack trace (to an older frame).
        """

        try:
            count = int(arg or 1)
        except ValueError:
            self.error("Invalid count")
            return False

        self.select_frame(-count, offset=True)
        self._print_stack_entry(self.get_current_frame())
        return False

    do_u = do_up

    def do_where(self, arg):
        try:
            stack = self.get_stack()
            prefix_size = len(str(len(stack)))
            for idx, frame in enumerate(stack):
                if frame is self.get_current_frame():
                    tag = '>'
                else:
                    tag = '#'
                self._print_stack_entry(frame,
                                        stack_prefix=f"{tag}{idx: <{prefix_size}} ",
                                        code_prefix=f"{'': >{prefix_size}}  -> ")
        except KeyboardInterrupt:
            pass
        return False

    do_w = do_where


def break_here():
    pdb = Pdbx()
    pdb.break_here(sys._getframe().f_back)

_usage = """\
usage: pdbx.py [-m module | pyfile] [arg] ...

Debug the Python program given by pyfile. Alternatively,
an executable module or package to debug can be specified using
the -m switch.
"""


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-m', nargs='?', default=None)
    options, commands = parser.parse_known_args()

    if options.m:
        # Run module
        target = _ModuleTarget(options.m)
        sys.argv[:] = [target.filename] + commands
    elif commands:
        # Run script
        target = _ScriptTarget(commands[0])
        sys.argv[:] = commands
    else:
        # Show help message
        print(_usage)
        sys.exit(2)

    pdbx = Pdbx()
    while True:
        try:
            pdbx._run_target(target)
        except SystemExit as e:
            # In most cases SystemExit does not warrant a post-mortem session.
            print("The program exited via sys.exit(). Exit status:", end=' ')
            print(e)
        except BdbxQuit:
            break


if __name__ == '__main__':
    import pdbx
    pdbx.main()
