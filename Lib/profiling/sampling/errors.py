"""Custom exceptions for the sampling profiler."""

class SamplingProfilerError(Exception):
    """Base exception for sampling profiler errors."""

class SamplingUnknownProcessError(SamplingProfilerError):
    def __init__(self, pid):
        self.pid = pid
        super().__init__(f"Process with PID '{pid}' does not exist.")

class SamplingScriptNotFoundError(SamplingProfilerError):
    def __init__(self, script_path):
        self.script_path = script_path
        super().__init__(f"Script '{script_path}' not found.")

class SamplingModuleNotFoundError(SamplingProfilerError):
    def __init__(self, module_name):
        self.module_name = module_name
        super().__init__(f"Module '{module_name}' not found.")
