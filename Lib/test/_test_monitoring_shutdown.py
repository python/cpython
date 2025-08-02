#!/usr/bin/env python3

# gh-115832: An object destructor running during the final GC of interpreter
# shutdown triggered an infinite loop in the instrumentation code.

import sys

class CallableCycle:
    def __init__(self):
        self._cycle = self

    def __del__(self):
        pass

    def __call__(self, code, instruction_offset):
        pass

def tracefunc(frame, event, arg):
    pass

def main():
    tool_id = sys.monitoring.PROFILER_ID
    event_id = sys.monitoring.events.PY_START

    sys.monitoring.use_tool_id(tool_id, "test profiler")
    sys.monitoring.set_events(tool_id, event_id)
    sys.monitoring.register_callback(tool_id, event_id, CallableCycle())

if __name__ == "__main__":
    sys.exit(main())
