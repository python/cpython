"""Mock classes for sampling profiler tests."""

from collections import namedtuple

# Matches the C structseq LocationInfo from _remote_debugging
LocationInfo = namedtuple('LocationInfo', ['lineno', 'end_lineno', 'col_offset', 'end_col_offset'])


class MockFrameInfo:
    """Mock FrameInfo for testing.

    Frame format: (filename, location, funcname, opcode) where:
    - location is a tuple (lineno, end_lineno, col_offset, end_col_offset)
    - opcode is an int or None
    """

    def __init__(self, filename, lineno, funcname, opcode=None):
        self.filename = filename
        self.funcname = funcname
        self.opcode = opcode
        self.location = LocationInfo(lineno, lineno, -1, -1)

    def __iter__(self):
        return iter((self.filename, self.location, self.funcname, self.opcode))

    def __getitem__(self, index):
        return (self.filename, self.location, self.funcname, self.opcode)[index]

    def __len__(self):
        return 4

    def __repr__(self):
        return f"MockFrameInfo('{self.filename}', {self.location}, '{self.funcname}', {self.opcode})"


class MockThreadInfo:
    """Mock ThreadInfo for testing since the real one isn't accessible."""

    def __init__(
        self, thread_id, frame_info, status=0
    ):  # Default to THREAD_STATE_RUNNING (0)
        self.thread_id = thread_id
        self.frame_info = frame_info
        self.status = status

    def __repr__(self):
        return f"MockThreadInfo(thread_id={self.thread_id}, frame_info={self.frame_info}, status={self.status})"


class MockInterpreterInfo:
    """Mock InterpreterInfo for testing since the real one isn't accessible."""

    def __init__(self, interpreter_id, threads):
        self.interpreter_id = interpreter_id
        self.threads = threads

    def __repr__(self):
        return f"MockInterpreterInfo(interpreter_id={self.interpreter_id}, threads={self.threads})"


class MockCoroInfo:
    """Mock CoroInfo for testing async tasks."""

    def __init__(self, task_name, call_stack):
        self.task_name = task_name  # In reality, this is the parent task ID
        self.call_stack = call_stack

    def __repr__(self):
        return f"MockCoroInfo(task_name={self.task_name}, call_stack={self.call_stack})"


class MockTaskInfo:
    """Mock TaskInfo for testing async tasks."""

    def __init__(self, task_id, task_name, coroutine_stack, awaited_by=None):
        self.task_id = task_id
        self.task_name = task_name
        self.coroutine_stack = coroutine_stack  # List of CoroInfo objects
        self.awaited_by = awaited_by or []  # List of CoroInfo objects (parents)

    def __repr__(self):
        return f"MockTaskInfo(task_id={self.task_id}, task_name={self.task_name})"


class MockAwaitedInfo:
    """Mock AwaitedInfo for testing async tasks."""

    def __init__(self, thread_id, awaited_by):
        self.thread_id = thread_id
        self.awaited_by = awaited_by  # List of TaskInfo objects

    def __repr__(self):
        return f"MockAwaitedInfo(thread_id={self.thread_id}, awaited_by={len(self.awaited_by)} tasks)"
