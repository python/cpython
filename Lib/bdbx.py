import cmd
import dis
import linecache
import os
import re
import reprlib
import sys
import tokenize

from dataclasses import dataclass
from types import FrameType, CodeType
from typing import Callable


MEVENT = sys.monitoring.events


class BdbxSetBreakpointException(Exception):
    pass


class Breakpoint:
    def __init__(self, file, line_number, code):
        self.file = file
        self.line_number = line_number
        self.code = code 

    def belong_to(self, code):
        if self.file == code.co_filename and \
                self.line_number >= code.co_firstlineno:
            for instr in dis.get_instructions(code):
                if instr.positions.lineno == self.line_number:
                    return True
        return False


class MonitorGenie:
    """
    MonitorGenie is a layer to handle PEP-669 events aka sys.monitoring.

    It saves the trouble for the debugger to handle the monitoring events.
    MonitorGenie takes file and function breakpoints, and an action to start
    the monitoring. The accepted actions are:
        "step"
        "next"
        "return"
        "continue"
    """
    def __init__(
        self,
        tool_id: int,
        debugger_entry: Callable[[FrameType, Breakpoint | None, int, dict | None], None]
    ):
        self._action = None
        self._frame = None
        self._tool_id = tool_id
        self._tasks = []
        self._bound_breakpoints: dict[CodeType, list[Breakpoint]] = {}
        self._free_breakpoints: list[Breakpoint] = []
        self._debugger_entry = debugger_entry 
        self._code_with_events = set()
        sys.monitoring.use_tool_id(tool_id, "MonitorGenie")
        self._register_callbacks()

    # ========================================================================
    # ============================= Public API ===============================
    # ========================================================================

    def start_monitor(self, action: str, frame: FrameType):
        """starts monitoring with the given action and frame"""
        self._clear_monitor()
        self._try_bind_breakpoints(frame.f_code)
        self._set_events_for_breakpoints()
        self._action, self._frame = action, frame
        if action == "step":
            self._add_local_events(frame.f_code, MEVENT.LINE | MEVENT.CALL | MEVENT.PY_RETURN \
                                    | MEVENT.PY_YIELD)
        elif action == "next":
            self._add_local_events(frame.f_code, MEVENT.LINE | MEVENT.PY_RETURN | MEVENT.PY_YIELD)
            if frame.f_back:
                self._add_local_events(frame.f_back.f_code, MEVENT.LINE | MEVENT.PY_RETURN | MEVENT.PY_YIELD)
        elif action == "return":
            if frame.f_back:
                self._add_local_events(frame.f_back.f_code, MEVENT.LINE | MEVENT.PY_RETURN | MEVENT.PY_YIELD)
        elif action == "continue":
            pass
        sys.monitoring.restart_events()

    def add_breakpoint(self, breakpoint: Breakpoint):
        if breakpoint.code is None:
            self._free_breakpoints.append(breakpoint)
        else:
            if breakpoint.code not in self._bound_breakpoints:
                self._bound_breakpoints[breakpoint.code] = []
            self._bound_breakpoints[breakpoint.code].append(breakpoint)

    def remove_breakpoint(self, breakpoint: Breakpoint):
        if breakpoint.code is None:
            self._free_breakpoints.remove(breakpoint)
        else:
            self._bound_breakpoints[breakpoint.code].remove(breakpoint)

    # ========================================================================
    # ============================ Private API ===============================
    # ========================================================================

    def _clear_monitor(self):
        sys.monitoring.set_events(self._tool_id, 0)
        for code in self._code_with_events:
            sys.monitoring.set_local_events(self._tool_id, code, 0)

    def _add_global_events(self, events):
        curr_events = sys.monitoring.get_events(self._tool_id)
        sys.monitoring.set_events(self._tool_id, curr_events | events)

    def _add_local_events(self, code, events):
        curr_events = sys.monitoring.get_local_events(self._tool_id, code)
        self._code_with_events.add(code)
        sys.monitoring.set_local_events(self._tool_id, code, curr_events | events)

    def _set_events_for_breakpoints(self):
        if self._free_breakpoints:
            self._add_global_events(MEVENT.PY_START)
        for code, bp_list in self._bound_breakpoints.items():
            for breakpoint in bp_list:
                if breakpoint.line_number is not None:
                    self._add_local_events(code, MEVENT.LINE)
                else:
                    self._add_local_events(code, MEVENT.PY_START)

    def _try_bind_breakpoints(self, code):
        # copy the breakpoints so we can remove bp from it
        bp_dirty = False
        for bp in self._free_breakpoints[:]:
            if bp.belong_to(code):
                self.remove_breakpoint(bp)
                bp.code = code
                self.add_breakpoint(bp)
                bp_dirty = True
                break
        if bp_dirty:
            self._set_events_for_breakpoints()
            if not self._free_breakpoints:
                sys.monitoring.set_events(self._tool_id, 0)

    def _stophere(self, code):
        if self._action == "step":
            return True
        elif self._action == "next":
            return code == self._frame.f_code or code == self._frame.f_back.f_code
        elif self._action == "return":
            return code == self._frame.f_back.f_code
        return False

    def _breakhere(self, code, line_number):
        if code in self._bound_breakpoints:
            for bp in self._bound_breakpoints[code]:
                # There are two possible cases
                # the line_number could be a real line number and match
                # or the line_number is None which only be given by PY_START
                # and will match on function breakpoints
                if bp.line_number == line_number:
                    return bp
        return None

    # Callbacks for the real sys.monitoring

    def _register_callbacks(self):
        sys.monitoring.register_callback(self._tool_id, MEVENT.LINE, self._line_callback)
        sys.monitoring.register_callback(self._tool_id, MEVENT.CALL, self._call_callback)
        sys.monitoring.register_callback(self._tool_id, MEVENT.PY_START, self._start_callback)
        sys.monitoring.register_callback(self._tool_id, MEVENT.PY_RETURN, self._return_callback)

    def _line_callback(self, code, line_number):
        if bp := self._breakhere(code, line_number):
            self._start_debugger(sys._getframe().f_back, bp, MEVENT.LINE,
                                 {"code": code, "line_number": line_number})
        elif self._stophere(code):
            self._start_debugger(sys._getframe().f_back, None, MEVENT.LINE,
                                 {"code": code, "line_number": line_number})
        else:
            return sys.monitoring.DISABLE

    def _call_callback(self, code, instruction_offset, callable, arg0):
        # The only possible trigget for this is "step" action
        # If the callable is instrumentable, do it, otherwise ignore it
        code = None
        if hasattr(callable, "__code__"):
            code = callable.__code__
        elif hasattr(callable, "__call__"):
            try:
                code = callable.__call__.__func__.__code__
            except AttributeError:
                pass
        if code is not None:
            self._add_local_events(code, MEVENT.LINE)

    def _start_callback(self, code, instruction_offset):
        self._try_bind_breakpoints(code)
        if bp := self._breakhere(code, None):
            self._start_debugger(sys._getframe().f_back, bp, MEVENT.PY_START,
                                 {"code": code, "instruction_offset": instruction_offset})
        elif self._stophere(code):
            self._start_debugger(sys._getframe().f_back, None, MEVENT.PY_START,
                                 {"code": code, "instruction_offset": instruction_offset})
        else:
            return sys.monitoring.DISABLE

    def _return_callback(self, code, instruction_offset, retval):
        if self._stophere(code):
            self._start_debugger(sys._getframe().f_back, None, MEVENT.PY_RETURN,
                                 {"code": code, "instruction_offset": instruction_offset, "retval": retval})
        else:
            return sys.monitoring.DISABLE

    def _start_debugger(self, frame, breakpoint, event, args):
        self._debugger_entry(frame, breakpoint, event, args)


@dataclass
class StopEvent:
    frame: FrameType
    line_number: int
    is_call: bool = False
    is_return: bool = False


class Bdbx:
    """Bdbx is a singleton class that implements the debugger logic"""
    _instance = None

    def __new__(cls):
        if Bdbx._instance is None:
            instance = super().__new__(cls)
            instance._tool_id = sys.monitoring.DEBUGGER_ID
            instance._monitor_genie = MonitorGenie(instance._tool_id, instance.monitor_callback)
            Bdbx._instance = instance
        return Bdbx._instance

    def __init__(self):
        self._next_action = None
        self._next_action_frame = None
        self._stop_event = None
        self._stop_frame = None
        self._curr_frame = None
        self._main_pyfile = ''
        self.clear_breakpoints()

    # ========================================================================
    # ============================= Public API ===============================
    # ========================================================================

    def break_here(self, frame=None):
        """break into the debugger as soon as possible"""
        if frame is None:
            frame = sys._getframe().f_back
        self.set_action("next", frame)
        self._monitor_genie.start_monitor(self._next_action, self._next_action_frame)

    def set_action(self, action, frame=None):
        """Set the next action, if frame is None, use the current frame"""
        if frame is None:
            frame = self._curr_frame

        self._next_action = action
        self._next_action_frame = frame

    def set_function_breakpoint(self, func):
        if not hasattr(func, "__code__"):
            raise BdbxSetBreakpointException(f"{func} is not a valid function!")
        abspath = os.path.abspath(func.__code__.co_filename)
        if not abspath:
            raise BdbxSetBreakpointException(f"Cann't find the source file for {func}!")
        # Setting line_number to None for function breakpoints
        bp = Breakpoint(abspath, None, func.__code__)
        self._breakpoints.append(bp)
        self._monitor_genie.add_breakpoint(bp)

    def set_file_breakpoint(self, filename, line_number):
        abspath = self._lookupmodule(filename)
        if not abspath:
            abspath = filename
            #raise BdbxSetBreakpointException(f"{filename} is not a valid file name!")
        try:
            line_number = int(line_number)
        except ValueError:
            raise BdbxSetBreakpointException(f"{line_number} is not a valid line number!")
        bp = Breakpoint(abspath, line_number, None)
        self._breakpoints.append(bp)
        self._monitor_genie.add_breakpoint(bp)

    def clear_breakpoints(self):
        if hasattr(self, "_breakpoints"):
            for bp in self._breakpoints:
                self._monitor_genie.remove_breakpoint(bp)
        self._breakpoints = []

    # Data accessors

    def get_current_frame(self):
        """Get the current frame"""
        return self._curr_frame

    def get_stack(self):
        """Get the current stack"""
        return self._stack

    def get_breakpoints(self):
        """Get all the breakpoints"""
        return self._breakpoints

    # Interface to be implemented by the debugger

    def dispatch_event(self, event: StopEvent):
        pass

    # communication with MonitorGenie

    def monitor_callback(self, frame, breakpoint, event, event_arg):
        """Callback entry from MonitorGenie"""

        self._curr_breakpoint = breakpoint
        self._stop_frame = frame
        self._curr_frame = frame
        self._stack = self._get_stack_from_frame(frame)

        if event == MEVENT.LINE:
            self._stop_event = StopEvent(frame, event_arg["line_number"])
        elif event == MEVENT.PY_START or event == MEVENT.PY_RESUME:
            self._stop_event = StopEvent(frame, 0, is_call=True)
        elif event == MEVENT.PY_RETURN or event == MEVENT.PY_YIELD:
            self._stop_event = StopEvent(frame, 0, is_return=True)
        else:
            raise RuntimeError("Not supposed to be here")

        self.dispatch_event(self._stop_event)

        # After the dispatch returns, reset the monitor
        self._monitor_genie.start_monitor(self._next_action, self._next_action_frame)

    # ========================================================================
    # ======================= Helper functions ===============================
    # ========================================================================

    def _get_stack_from_frame(self, frame):
        """Get call stack from the latest frame, oldest frame at [0]"""
        stack = []
        while frame:
            stack.append(frame)
            frame = frame.f_back
        return reversed(stack)

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


class Pdbx(Bdbx, cmd.Cmd):
    def __init__(self):
        self._event = None
        self.prompt = "(Pdbx) "
        self.fncache = {}
        Bdbx.__init__(self)
        cmd.Cmd.__init__(self, 'tab', None, None)

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

    @property
    def _default_file(self):
        """Produce a reasonable default."""
        filename = self.get_current_frame().f_code.co_filename
        if filename == '<string>' and self._main_pyfile:
            filename = self._main_pyfile
        return filename

    def _format_stack_entry(self, frame, lineno, lprefix=': '):
        """Return a string with information about a stack entry.

        The stack entry frame_lineno is a (frame, lineno) tuple.  The
        return string contains the canonical filename, the function name
        or '<lambda>', the input arguments, the return value, and the
        line of code (if it exists).

        """
        filename = self._canonic(frame.f_code.co_filename)
        s = '%s(%r)' % (filename, lineno)
        if frame.f_code.co_name:
            s += frame.f_code.co_name
        else:
            s += "<lambda>"
        s += '()'
        if '__return__' in frame.f_locals:
            rv = frame.f_locals['__return__']
            s += '->'
            s += reprlib.repr(rv)
        line = linecache.getline(filename, lineno, frame.f_globals)
        if line:
            s += lprefix + line.strip()
        return s

    def _print_stack_entry(self, frame, lineno):
        if frame is self.get_current_frame():
            prefix = '> '
        else:
            prefix = '  '
        self.message(prefix +
                     self._format_stack_entry(frame, lineno, '\n-> '))

    def _checkline(self, filename, lineno):
        """Check whether specified line seems to be executable.

        Return `lineno` if it is, 0 if not (e.g. a docstring, comment, blank
        line or EOF). Warning: testing is not comprehensive.
        """
        # this method should be callable before starting debugging, so default
        # to "no globals" if there is no current frame
        globs = self.get_current_frame().f_globals if self.get_current_frame() else None
        line = linecache.getline(filename, lineno, globs)
        if not line:
            self.message('End of file')
            return 0
        line = line.strip()
        # Don't allow setting breakpoint at a blank line
        if (not line or (line[0] == '#') or
             (line[:3] == '"""') or line[:3] == "'''"):
            self.error('Blank or comment')
            return 0
        return lineno

    def _lineinfo(self, identifier):
        failed = (None, None, None)
        # Input is identifier, may be in single quotes
        idstring = identifier.split("'")
        if len(idstring) == 1:
            # not in single quotes
            id = idstring[0].strip()
        elif len(idstring) == 3:
            # quoted
            id = idstring[1].strip()
        else:
            return failed
        if id == '': return failed
        parts = id.split('.')
        # Protection for derived debuggers
        if parts[0] == 'self':
            del parts[0]
            if len(parts) == 0:
                return failed
        # Best first guess at file to look at
        fname = self.defaultFile()
        if len(parts) == 1:
            item = parts[0]
        else:
            # More than one part.
            # First is module, second is method/class
            f = self._lookupmodule(parts[0])
            if f:
                fname = f
            item = parts[1]
        answer = self._find_function(item, fname)
        return answer or failed

    def _find_function(funcname, filename):
        cre = re.compile(r'def\s+%s\s*[(]' % re.escape(funcname))
        try:
            fp = tokenize.open(filename)
        except OSError:
            return None
        # consumer of this info expects the first line to be 1
        with fp:
            for lineno, line in enumerate(fp, start=1):
                if cre.match(line):
                    return funcname, filename, lineno
        return None

    # ======================================================================
    # The following methods are called by the cmd.Cmd base class
    # All the commands are in alphabetic order
    # ======================================================================

    def do_break(self, arg):
        if not arg:
            print(self.get_breakpoints())
            return False
        # parse arguments; comma has lowest precedence
        # and cannot occur in filename
        filename = None
        lineno = None
        # parse stuff before comma: [filename:]lineno | function
        colon = arg.rfind(':')
        if colon >= 0:
            filename = arg[:colon].rstrip()
            line_number = arg[colon+1:]
            try:
                self.set_file_breakpoint(filename, line_number)
            except BdbxSetBreakpointException as e:
                self.error(e)
                return False
        else:
            # no colon; can be lineno or function
            try:
                lineno = int(arg)
            except ValueError:
                try:
                    frame = self.get_current_frame()
                    func = eval(arg,
                                frame.f_globals,
                                frame.f_locals)
                except:
                    func = arg
                    (ok, filename, line_number) = self._lineinfo(arg)
                    if not ok:
                        self.error(f"Can't find function '{arg}'")
                        return False
                    try:
                        self.set_file_breakpoint(filename, line_number)
                    except BdbxSetBreakpointException as e:
                        self.error(e)
                    return False
                try:
                    self.set_function_breakpoint(func)
                except BdbxSetBreakpointException as e:
                    self.error(e)
                    return False
                return False
        if not filename:
            filename = self._default_file
        # Check for reasonable breakpoint
        line = self._checkline(filename, lineno)
        if line:
            # now set the break point
            self.set_file_breakpoint(filename, line)
        return False

    do_b = do_break

    def do_clear(self, arg):
        self.clear_breakpoints()
        return False

    def do_continue(self, arg):
        self.set_action("continue")
        return True

    do_c = do_continue

    def do_next(self, arg):
        self.set_action("next")
        return True

    do_n = do_next

    def do_quit(self, arg):
        raise Exception("quit")

    do_q = do_quit

    def do_return(self, arg):
        self.set_action("return")
        return True

    do_r = do_return

    def do_step(self, arg):
        self.set_action("step")
        return True

    do_s = do_step

    def do_where(self, arg):
        try:
            for frame in self.get_stack():
                lineno = frame.f_lineno
                self._print_stack_entry(frame, lineno)
        except KeyboardInterrupt:
            pass
        return False

    do_w = do_where


def break_here():
    pdb = Pdbx()
    pdb.break_here(sys._getframe().f_back)
