import dis
import os
import sys

from dataclasses import dataclass
from types import FrameType, CodeType
from typing import Callable


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
        self._returning = False
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
        sys.monitoring.restart_events()

    def start_monitor_code(self, code: CodeType):
        """starts monitoring when the given code is executed"""
        self._clear_monitor()
        self._action, self._frame = "step", None
        self._add_local_events(code, MEVENT.LINE | MEVENT.CALL | MEVENT.PY_RETURN | MEVENT.PY_YIELD)
        sys.monitoring.restart_events()

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
        sys.monitoring.register_callback(self._tool_id, MEVENT.PY_YIELD, self._return_callback)
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
            self._returning = True
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

    def break_code(self, code):
        """break into the debugger when the given code is executed"""
        self.set_action("step", None)
        self._monitor_genie.start_monitor_code(code)

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
        """Set a breakpoint at the given line number in the given file

            The caller is responsible for checking that the file exists and
            that the line number is valid.
        """
        bp = Breakpoint(filename, line_number, None)
        self._breakpoints.append(bp)
        self._monitor_genie.add_breakpoint(bp)

    def clear_breakpoints(self):
        if hasattr(self, "_breakpoints"):
            for bp in self._breakpoints:
                self._monitor_genie.remove_breakpoint(bp)
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
        stack.reverse()
        return stack
