"""Common test helpers and mocks for live collector tests."""

from collections import namedtuple

from profiling.sampling.constants import (
    THREAD_STATUS_HAS_GIL,
    THREAD_STATUS_ON_CPU,
)


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
    """Mock ThreadInfo for testing."""

    def __init__(self, thread_id, frame_info, status=THREAD_STATUS_HAS_GIL | THREAD_STATUS_ON_CPU):
        self.thread_id = thread_id
        self.frame_info = frame_info
        self.status = status

    def __repr__(self):
        return f"MockThreadInfo(thread_id={self.thread_id}, frame_info={self.frame_info}, status={self.status})"


class MockInterpreterInfo:
    """Mock InterpreterInfo for testing."""

    def __init__(self, interpreter_id, threads):
        self.interpreter_id = interpreter_id
        self.threads = threads

    def __repr__(self):
        return f"MockInterpreterInfo(interpreter_id={self.interpreter_id}, threads={self.threads})"
