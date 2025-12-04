"""Mock classes for sampling profiler tests."""


class MockFrameInfo:
    """Mock FrameInfo for testing since the real one isn't accessible."""

    def __init__(self, filename, lineno, funcname):
        self.filename = filename
        self.lineno = lineno
        self.funcname = funcname

    def __repr__(self):
        return f"MockFrameInfo(filename='{self.filename}', lineno={self.lineno}, funcname='{self.funcname}')"


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
