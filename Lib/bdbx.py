import dis
import os
import sys

from dataclasses import dataclass
from types import FrameType, CodeType


MEVENT = sys.monitoring.events


class BdbxSetBreakpointException(Exception):
    pass


class BdbxQuit(Exception):
    pass


class Breakpoint:
    _next_id = 1

    def __init__(self, file: str, line_number: int, code: CodeType | None = None):
        self._id = self._get_next_id()
        self.file = file
        self.line_number = line_number
        self.code = code 

    def belong_to(self, code: CodeType):
        """returns True if the breakpoint belongs to the given code object"""
        if self.file == code.co_filename and \
                self.line_number >= code.co_firstlineno:
            for instr in dis.get_instructions(code):
                if instr.positions.lineno == self.line_number:
                    return True
        return False

    def __str__(self):
        return f"Breakpoint {self._id} at {self.file}:{self.line_number}"

    @classmethod
    def _get_next_id(cls):
        cls._next_id += 1
        return cls._next_id - 1


class _DebuggerMonitor:
    def __init__(
        self,
        debugger,
    ):
        self._action = None
        self._frame = None
        self._debugger = debugger
        self._returning = False
        self._bound_breakpoints: dict[CodeType, list[Breakpoint]] = {}
        self._free_breakpoints: list[Breakpoint] = []
        self._code_with_events: dict[CodeType, int] = {}

    @property
    def has_breakpoint(self):
        return self._free_breakpoints or self._bound_breakpoints

    def get_global_events(self):
        if self._free_breakpoints:
            return MEVENT.CALL
        else:
            return MEVENT.NO_EVENTS

    def get_local_events(self):
        for code, bp_list in self._bound_breakpoints.items():
            for breakpoint in bp_list:
                if breakpoint.line_number is not None:
                    self._add_local_events(code, MEVENT.LINE)
                else:
                    self._add_local_events(code, MEVENT.PY_START)
        return self._code_with_events

    def clear_events(self):
        self._code_with_events = {}

    def set_action(self, action: str, frame: FrameType):
        self._action = action
        self._frame = frame
        if action == "step":
            if not self._returning:
                self._add_local_events(frame.f_code, MEVENT.LINE | MEVENT.CALL | MEVENT.PY_RETURN \
                                       | MEVENT.PY_YIELD)
            elif frame.f_back:
                self._add_local_events(frame.f_back.f_code, MEVENT.LINE | MEVENT.CALL | MEVENT.PY_RETURN \
                                       | MEVENT.PY_YIELD)
        elif action == "next":
            if not self._returning:
                self._add_local_events(frame.f_code, MEVENT.LINE | MEVENT.PY_RETURN | MEVENT.PY_YIELD)
            elif frame.f_back:
                self._add_local_events(frame.f_back.f_code, MEVENT.LINE | MEVENT.PY_RETURN | MEVENT.PY_YIELD)
        elif action == "return":
            if frame.f_back:
                self._add_local_events(frame.f_back.f_code, MEVENT.LINE | MEVENT.PY_RETURN | MEVENT.PY_YIELD)
        elif action == "continue":
            pass
        self._returning = False

    def set_code(self, code: CodeType):
        self._action = "step"
        self.frame = None
        self._add_local_events(code, MEVENT.LINE | MEVENT.CALL | MEVENT.PY_RETURN | MEVENT.PY_YIELD)

    def try_bind_breakpoints(self, code):
        """try to bind free breakpoint, return whether any breakpoint is bound"""
        bp_dirty = False
        for bp in self._free_breakpoints[:]:
            if bp.belong_to(code):
                self.remove_breakpoint(bp)
                bp.code = code
                self.add_breakpoint(bp)
                bp_dirty = True
                break
        return bp_dirty

    def add_breakpoint(self, breakpoint: Breakpoint):
        """adds a breakpoint to the list of breakpoints"""
        if breakpoint.code is None:
            self._free_breakpoints.append(breakpoint)
        else:
            if breakpoint.code not in self._bound_breakpoints:
                self._bound_breakpoints[breakpoint.code] = []
            self._bound_breakpoints[breakpoint.code].append(breakpoint)

    def remove_breakpoint(self, breakpoint: Breakpoint):
        """removes a breakpoint from the list of breakpoints"""
        if breakpoint.code is None:
            self._free_breakpoints.remove(breakpoint)
        else:
            self._bound_breakpoints[breakpoint.code].remove(breakpoint)
            if not self._bound_breakpoints[breakpoint.code]:
                del self._bound_breakpoints[breakpoint.code]

    def line_callback(self, frame, code, line_number):
        if bp := self._breakhere(code, line_number):
            self._debugger.monitor_callback(frame, bp, MEVENT.LINE,
                                            {"code": code, "line_number": line_number})
        elif self._stophere(code):
            self._debugger.monitor_callback(frame, None, MEVENT.LINE,
                                            {"code": code, "line_number": line_number})
        else:
            return False
        return True

    def call_callback(self, code):
        # The only possible trigget for this is "step" action
        # If the callable is instrumentable, do it, otherwise ignore it
        if self._action == "step":
            self._add_local_events(code, MEVENT.LINE)
            return True
        return False

    def start_callback(self, frame, code, instruction_offset):
        if bp := self._breakhere(code, None):
            self._debugger.monitor_callback(frame, bp, MEVENT.PY_START,
                                            {"code": code, "instruction_offset": instruction_offset})
        elif self._stophere(code):
            self._debugger.monitor_callback(frame, None, MEVENT.PY_START,
                                            {"code": code, "instruction_offset": instruction_offset})
        else:
            return False
        return True

    def return_callback(self, frame, code, instruction_offset, retval):
        if self._stophere(code):
            self._returning = True
            self._debugger.monitor_callback(frame, None, MEVENT.PY_RETURN,
                                            {"code": code, "instruction_offset": instruction_offset, "retval": retval})
        else:
            return False
        return True

    def _add_local_events(self, code, events):
        self._code_with_events[code] = self._code_with_events.get(code, 0) | events

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
    ):
        self._action = None
        self._frame = None
        self._tool_id = tool_id
        self._tasks = []
        sys.monitoring.use_tool_id(tool_id, "MonitorGenie")
        self._register_callbacks()
        self._debugger_monitors: dict[Bdbx, _DebuggerMonitor] = {}
        self._code_with_events = set()

    # ========================================================================
    # ============================= Public API ===============================
    # ========================================================================

    def register_debugger(self, debugger):
        if debugger not in self._debugger_monitors:
            self._debugger_monitors[debugger] = _DebuggerMonitor(debugger)

    def unregister_debugger(self, debugger):
        if debugger in self._debugger_monitors:
            self._debugger_monitors.pop(debugger)

    def start_monitor(self, debugger, action: str, frame: FrameType):
        """starts monitoring with the given action and frame"""
        dbg_monitor = self._get_monitor(debugger)

        if action == "continue" and not dbg_monitor.has_breakpoint:
            self.unregister_debugger(debugger)
        else:
            dbg_monitor.clear_events()
            dbg_monitor.try_bind_breakpoints(frame.f_code)
            dbg_monitor.set_action(action, frame)
        
        self._set_events()
        sys.monitoring.restart_events()

    def start_monitor_code(self, debugger, code: CodeType):
        """starts monitoring when the given code is executed"""
        dbg_monitor = self._get_monitor(debugger)
        dbg_monitor.clear_events()
        dbg_monitor.set_code(code)
        self._set_events()
        sys.monitoring.restart_events()

    def add_breakpoint(self, debugger, breakpoint: Breakpoint):
        self._get_monitor(debugger).add_breakpoint(breakpoint)

    def remove_breakpoint(self, debugger, breakpoint: Breakpoint):
        self._get_monitor(debugger).remove_breakpoint(breakpoint)

    # ========================================================================
    # ============================ Private API ===============================
    # ========================================================================

    def _get_monitor(self, debugger):
        if debugger not in self._debugger_monitors:
            self.register_debugger(debugger)
        return self._debugger_monitors[debugger]

    def _get_monitors(self):
        return list(self._debugger_monitors.values())

    def _set_events(self):
        """
        Go through all the registered debuggers and figure out all the events
        that need to be set
        """
        self._clear_monitor()
        global_events = MEVENT.NO_EVENTS
        for dbg_monitor in self._get_monitors():
            global_events |= dbg_monitor.get_global_events()
            for code, events in dbg_monitor.get_local_events().items():
                self._add_local_events(code, events)

    def _clear_monitor(self):
        sys.monitoring.set_events(self._tool_id, 0)
        for code in self._code_with_events:
            sys.monitoring.set_local_events(self._tool_id, code, 0)
        self._code_with_events = set()

    def _add_global_events(self, events):
        curr_events = sys.monitoring.get_events(self._tool_id)
        sys.monitoring.set_events(self._tool_id, curr_events | events)

    def _add_local_events(self, code, events):
        curr_events = sys.monitoring.get_local_events(self._tool_id, code)
        self._code_with_events.add(code)
        sys.monitoring.set_local_events(self._tool_id, code, curr_events | events)

    # Callbacks for the real sys.monitoring

    def _register_callbacks(self):
        sys.monitoring.register_callback(self._tool_id, MEVENT.LINE, self._line_callback)
        sys.monitoring.register_callback(self._tool_id, MEVENT.CALL, self._call_callback)
        sys.monitoring.register_callback(self._tool_id, MEVENT.PY_START, self._start_callback)
        sys.monitoring.register_callback(self._tool_id, MEVENT.PY_YIELD, self._return_callback)
        sys.monitoring.register_callback(self._tool_id, MEVENT.PY_RETURN, self._return_callback)

    def _line_callback(self, code, line_number):
        frame = sys._getframe(1)
        triggered_callback = False
        for dbg_monitor in self._get_monitors():
            triggered_callback |= dbg_monitor.line_callback(frame, code, line_number)
        if not triggered_callback:
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
            for dbg_monitor in self._get_monitors():
                if dbg_monitor.call_callback(code):
                    self._set_events()

    def _start_callback(self, code, instruction_offset):
        frame = sys._getframe(1)
        triggered_callback = False
        for dbg_monitor in self._get_monitors():
            if dbg_monitor.try_bind_breakpoints(code):
                self._set_events()
            triggered_callback |= dbg_monitor.start_callback(frame, code, instruction_offset)
        if not triggered_callback:
            return sys.monitoring.DISABLE

    def _return_callback(self, code, instruction_offset, retval):
        frame = sys._getframe(1)
        triggered_callback = False
        for dbg_monitor in self._get_monitors():
            triggered_callback |= dbg_monitor.return_callback(frame, code, instruction_offset, retval)
        if not triggered_callback:
            return sys.monitoring.DISABLE


_monitor_genie = MonitorGenie(sys.monitoring.DEBUGGER_ID)

@dataclass
class StopEvent:
    frame: FrameType
    line_number: int
    is_call: bool = False
    is_return: bool = False

class Bdbx:
    def __init__(self):
        self._next_action = None
        self._next_action_frame = None
        self._stop_event = None
        self._stop_frame = None
        self._curr_frame = None
        self._main_pyfile = ''
        self.clear_breakpoints()
        self._monitor_genie = _monitor_genie

    # ========================================================================
    # ============================= Public API ===============================
    # ========================================================================

    def break_here(self, frame=None):
        """break into the debugger as soon as possible"""
        if frame is None:
            frame = sys._getframe().f_back
        self.set_action("next", frame)
        self._monitor_genie.start_monitor(self, self._next_action, self._next_action_frame)

    def break_code(self, code):
        """break into the debugger when the given code is executed"""
        self.set_action("step", None)
        self._monitor_genie.start_monitor_code(self, code)

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
        self._monitor_genie.add_breakpoint(self, bp)

    def set_file_breakpoint(self, filename, line_number):
        """Set a breakpoint at the given line number in the given file

            The caller is responsible for checking that the file exists and
            that the line number is valid.
        """
        bp = Breakpoint(filename, line_number, None)
        self._breakpoints.append(bp)
        self._monitor_genie.add_breakpoint(self, bp)

    def clear_breakpoints(self):
        if hasattr(self, "_breakpoints"):
            for bp in self._breakpoints:
                self._monitor_genie.remove_breakpoint(self, bp)
        self._breakpoints = []

    def select_frame(self, index, offset=False):
        """Select a frame in the stack"""
        if offset:
            index += self._curr_frame_idx
        if index < 0:
            index = 0
        elif index >= len(self._stack):
            index = len(self._stack) - 1
        self._curr_frame_idx = index
        self._curr_frame = self._stack[index]

    # Data accessors

    def get_current_frame(self):
        """Get the current frame"""
        return self._curr_frame

    def get_current_frame_idx(self):
        """Get the current frame index"""
        return self._curr_frame_idx

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
        self._curr_frame_idx = len(self._stack) - 1

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
        self._monitor_genie.start_monitor(self, self._next_action, self._next_action_frame)

    # ========================================================================
    # ======================= Helper functions ===============================
    # ========================================================================

    def _get_stack_from_frame(self, frame):
        """Get call stack from the latest frame, oldest frame at [0]"""
        stack = []
        while frame:
            stack.append(frame)
            frame = frame.f_back
        stack.reverse()
        return stack
